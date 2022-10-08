#pragma once
#include "../core/foray_deviceresource.hpp"
#include "../core/foray_vkcontext.hpp"
#include "../bench/foray_bench_declares.hpp"
#include "../core/foray_managedbuffer.hpp"
#include "../scene/foray_scene_declares.hpp"

namespace foray::as {

    /// @brief A Blas (Bottom Level Acceleration Structure) is the raytracing equivalent concept of a mesh
    class Blas : public core::DeviceResourceBase
    {
      public:
        virtual ~Blas() { Destroy(); }

        virtual void CreateOrUpdate(const core::VkContext* context, const scene::Mesh* mesh, const scene::GeometryStore* store, bench::HostBenchmark* benchmark = nullptr);

        inline virtual bool Exists() const override { return !mAccelerationStructure; }
        virtual void        Destroy() override;

        FORAY_PROPERTY_CGET(AccelerationStructure)
        FORAY_PROPERTY_CGET(BlasAddress)
        FORAY_PROPERTY_CGET(BlasMemory)
        FORAY_PROPERTY_CGET(Mesh)

      protected:
        const core::VkContext*           mContext = nullptr;
        const scene::Mesh*                mMesh    = nullptr;
        core::ManagedBuffer              mBlasMemory;
        VkAccelerationStructureKHR mAccelerationStructure{};
        VkDeviceAddress            mBlasAddress{};
    };
}  // namespace foray