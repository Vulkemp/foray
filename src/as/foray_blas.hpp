#pragma once
#include "../bench/foray_bench_declares.hpp"
#include "../core/foray_context.hpp"
#include "../core/foray_managedbuffer.hpp"
#include "../core/foray_managedresource.hpp"
#include "../scene/foray_scene_declares.hpp"

namespace foray::as {

    /// @brief A Blas (Bottom Level Acceleration Structure) is the raytracing equivalent concept of a mesh
    class Blas : public core::VulkanResource<VkObjectType::VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR>
    {
      public:
        virtual ~Blas() { Destroy(); }

        inline virtual std::string_view GetTypeName() const override { return "Bottom-Level AS"; }

        virtual void CreateOrUpdate(core::Context* context, const scene::Mesh* mesh, const scene::GeometryStore* store, bench::HostBenchmark* benchmark = nullptr);

        inline virtual bool Exists() const override { return !mAccelerationStructure; }
        virtual void        Destroy() override;

        FORAY_PROPERTY_CGET(AccelerationStructure)
        FORAY_PROPERTY_CGET(BlasAddress)
        FORAY_PROPERTY_CGET(BlasMemory)
        FORAY_PROPERTY_CGET(Mesh)

      protected:
        core::Context*             mContext = nullptr;
        const scene::Mesh*         mMesh    = nullptr;
        core::ManagedBuffer        mBlasMemory;
        VkAccelerationStructureKHR mAccelerationStructure{};
        VkDeviceAddress            mBlasAddress{};
    };
}  // namespace foray::as