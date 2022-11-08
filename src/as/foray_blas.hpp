#pragma once
#include "../bench/foray_bench_declares.hpp"
#include "../core/foray_context.hpp"
#include "../core/foray_managedbuffer.hpp"
#include "../core/foray_managedresource.hpp"
#include "../scene/foray_scene_declares.hpp"

namespace foray::as {

    /// @brief A Blas (Bottom Level Acceleration Structure) is the raytracing equivalent concept of a mesh
    /// @details
    /// This class takes the geometry referenced by a mesh from a geometrystore, and builds a bottom level acceleration structure from it.
    /// It manages (owns) both the BLAs buffer aswell as the handle and exposes the Blas' device address.
    class Blas : public core::VulkanResource<VkObjectType::VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR>
    {
      public:
        virtual ~Blas() { Destroy(); }

        inline static const char* BENCH_RESET         = "Reset";
        inline static const char* BENCH_CREATESTRUCTS = "Create Build Structs";
        inline static const char* BENCH_GETSIZES = "Get Build Sizes";
        inline static const char* BENCH_CREATE = "Create";
        inline static const char* BENCH_BUILD = "Build";

        inline virtual std::string_view GetTypeName() const override { return "Bottom-Level AS"; }

        /// @brief Recreates the acceleration structure
        /// @details For each primitive in mesh, creates a geometry structure and corresponding build range.
        /// See static BENCH_... members for benchmark timestamps
        /// @param context Requires DispatchTable, PhysicalDevice, LogicalDevice
        /// @param mesh Required mesh object referencing the geometry
        /// @param store Required GeometryStore owning the Vertex+Index buffers
        /// @param benchmark Optional benchmark for timing the build process
        virtual void CreateOrUpdate(core::Context* context, const scene::Mesh* mesh, const scene::gcomp::GeometryStore* store, bench::HostBenchmark* benchmark = nullptr);

        inline virtual bool Exists() const override { return !mAccelerationStructure; }
        virtual void        Destroy() override;

        FORAY_GETTER_V(AccelerationStructure)
        FORAY_GETTER_V(BlasAddress)
        FORAY_GETTER_CR(BlasMemory)
        FORAY_GETTER_V(Mesh)

      protected:
        core::Context*             mContext = nullptr;
        const scene::Mesh*         mMesh    = nullptr;
        core::ManagedBuffer        mBlasMemory;
        VkAccelerationStructureKHR mAccelerationStructure{};
        VkDeviceAddress            mBlasAddress{};
    };
}  // namespace foray::as