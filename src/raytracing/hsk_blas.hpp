#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "../bench/hsk_bench_declares.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include "../scenegraph/hsk_scenegraph_declares.hpp"

namespace hsk {

    // hsk_geo Vertex struct prototype
    struct Vertex;
    class Mesh;

    /// @brief A Blas (Bottom Level Acceleration Structure) is the raytracing equivalent concept of a mesh
    class Blas : public DeviceResourceBase
    {
      public:
        virtual ~Blas() { Destroy(); }

        virtual void CreateOrUpdate(const VkContext* context, const Mesh* mesh, const GeometryStore* store, HostBenchmark* benchmark = nullptr);

        inline virtual bool Exists() const override { return !mAccelerationStructure; }
        virtual void        Destroy() override;

        HSK_PROPERTY_CGET(AccelerationStructure)
        HSK_PROPERTY_CGET(BlasAddress)
        HSK_PROPERTY_CGET(BlasMemory)
        HSK_PROPERTY_CGET(Mesh)

      protected:
        const VkContext*           mContext = nullptr;
        const Mesh*                mMesh    = nullptr;
        ManagedBuffer              mBlasMemory;
        VkAccelerationStructureKHR mAccelerationStructure{};
        VkDeviceAddress            mBlasAddress{};
    };
}  // namespace hsk