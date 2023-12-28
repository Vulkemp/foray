#pragma once
#include "../bench/bench_declares.hpp"
#include "../core/context.hpp"
#include "../core/managedbuffer.hpp"
#include "../core/managedresource.hpp"
#include "../mem.hpp"
#include "../scene/scene_declares.hpp"

namespace foray::as {

    /// @brief A Blas (Bottom Level Acceleration Structure) is the raytracing equivalent concept of a mesh
    /// @details
    /// This class takes the geometry referenced by a mesh from a geometrystore, and builds a bottom level acceleration structure from it.
    /// It manages (owns) both the BLAs buffer aswell as the handle and exposes the Blas' device address.
    class Blas : public core::VulkanResource<vk::ObjectType::eAccelerationStructureKHR>
    {
      public:
        class Builder
        {
          public:
            Builder& SetGeometryStore(scene::gcomp::GeometryStore* store);

            FORAY_PROPERTY_V(Mesh)
            FORAY_PROPERTY_V(VertexBuffer)
            FORAY_PROPERTY_V(IndexBuffer)
            FORAY_PROPERTY_V(UpdateAfterBuild)
          protected:
            scene::Mesh*         mMesh             = nullptr;
            core::ManagedBuffer* mVertexBuffer     = nullptr;
            core::ManagedBuffer* mIndexBuffer      = nullptr;
            bool                 mUpdateAfterBuild = false;
        };

        /// @brief Recreates the acceleration structure
        /// @details For each primitive in mesh, creates a geometry structure and corresponding build range.
        /// See static BENCH_... members for benchmark timestamps
        /// @param context Requires DispatchTable, PhysicalDevice, LogicalDevice
        /// @param mesh Required mesh object referencing the geometry
        /// @param store Required GeometryStore owning the Vertex+Index buffers
        /// @param benchmark Optional benchmark for timing the build process
        Blas(core::Context* context, const Builder& builder, bench::HostBenchmark* benchmark = nullptr);
        virtual ~Blas();

        inline static const char* BENCH_CREATESTRUCTS = "Create Build Structs";
        inline static const char* BENCH_GETSIZES      = "Get Build Sizes";
        inline static const char* BENCH_CREATE        = "Create";
        inline static const char* BENCH_BUILD         = "Build";
        inline static const char* BENCH_GETADDRESS    = "Get AS Address";

        inline virtual std::string_view GetTypeName() const override { return "Bottom-Level AS"; }

        /// @brief GPU only update from the same information as previously used during creation / full update (buffer addresses may change)
        /// @param cmdBuffer Command buffer to write build commands to
        virtual void CmdUpdate(VkCommandBuffer cmdBuffer);

        FORAY_GETTER_V(AccelerationStructure)
        FORAY_GETTER_V(BlasAddress)
        FORAY_GETTER_MEM(BuildInfo)
        FORAY_GETTER_MEM(BlasMemory)
        FORAY_GETTER_MEM(ScratchMemory)
        FORAY_GETTER_V(Mesh)

      protected:
        struct BuildInfo
        {
            BuildInfo(core::Context* context, const scene::Mesh* mesh, const core::ManagedBuffer* vertexBuffer, const core::ManagedBuffer* indexBuffer);
            /// @brief Number of primitives (aka individual draw calls) for the contained mesh
            uint32_t PrimitiveCount;
            /// @brief Per primitive info of VkCmdDrawIndexed-like parameters
            std::vector<VkAccelerationStructureBuildRangeInfoKHR> BuildRangeInfos;
            /// @brief Counts of triangles per geometry, used to determine build size of the BLAS
            std::vector<uint32_t> TriangleCounts;
            /// @brief Additional information per geometry/primitive (Vertex format+stride+bufferaddress, Index format+bufferaddress, transformation data)
            std::vector<VkAccelerationStructureGeometryKHR> Geometries;
            /// @brief Final build info
            VkAccelerationStructureBuildGeometryInfoKHR GeoInfo;
            /// @brief Info about the required buffer sizes for building/updating
            VkAccelerationStructureBuildSizesInfoKHR Sizes;
        };

        core::Context*                                     mContext      = nullptr;
        const scene::Mesh*                                 mMesh         = nullptr;
        core::ManagedBuffer*                               mVertexBuffer = nullptr;
        core::ManagedBuffer*                               mIndexBuffer  = nullptr;
        Local<BuildInfo>                                   mBuildInfo;
        Local<core::ManagedBuffer>                         mBlasMemory;
        Local<core::ManagedBuffer>                         mScratchMemory;
        vk::AccelerationStructureKHR                         mAccelerationStructure = nullptr;
        vk::DeviceAddress                                    mBlasAddress           = {};
    };
}  // namespace foray::as