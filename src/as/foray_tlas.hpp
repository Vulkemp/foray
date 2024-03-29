#pragma once
#include "../core/foray_managedresource.hpp"
#include "../foray_basics.hpp"
#include "../scene/foray_component.hpp"
#include "../scene/foray_scene_declares.hpp"
#include "../util/foray_dualbuffer.hpp"
#include "foray_as_declares.hpp"
#include "foray_blasinstance.hpp"
#include "foray_geometrymetabuffer.hpp"
#include <map>
#include <vulkan/vulkan.h>

namespace foray::as {


    /// @brief Describes a top level accerlation structure. A tlas usually holds multiple Blas.
    /// A blas is an object/mesh instance together with its transformation in 3d space.
    class Tlas : public core::ManagedResource
    {
      public:
        Tlas() = default;
        virtual ~Tlas() { Destroy(); }

        inline virtual std::string_view GetTypeName() const override { return "Top-Level AS"; }

        /// @brief (Re)creates the TLAS. Required to invoke when changes to the TLAS transitioned it to Dirty state. Will synchronize with the CPU.
        virtual void CreateOrUpdate(core::Context* context = nullptr);
        /// @brief Updates transforms only. TLAS rebuild is performed and synchronized on GPU only. TLAS must by non-Dirty!
        virtual void UpdateLean(VkCommandBuffer cmdBuffer, uint32_t frameIndex);

        inline virtual bool Exists() const override { return !!mAccelerationStructure; }
        virtual void        Destroy() override;

        FORAY_GETTER_V(AccelerationStructure)
        FORAY_GETTER_CR(TlasMemory)
        FORAY_GETTER_V(TlasAddress)
        FORAY_GETTER_CR(MetaBuffer)
        FORAY_GETTER_V(Dirty)

        operator VkAccelerationStructureKHR() { return mAccelerationStructure; }

        /// @brief Remove a BLAS instance
        /// @remark TLAS will be marked Dirty!
        void RemoveBlasInstance(uint64_t id);
        /// @brief Get A BLAS instance by id
        const BlasInstance* GetBlasInstance(uint64_t id) const;
        /// @brief Add a BLAS instance from meshInstance component
        /// @remark TLAS will be marked Dirty!
        uint64_t AddBlasInstanceAuto(scene::ncomp::MeshInstance* meshInstance);
        /// @brief Add an animated BLAS instance
        /// @remark TLAS will be marked Dirty!
        uint64_t AddBlasInstanceAnimated(const Blas& blas, BlasInstance::TransformUpdateFunc getUpdatedGlobalTransformFunc);
        /// @brief Add a static BLAS instance
        /// @remark TLAS will be marked Dirty!
        uint64_t AddBlasInstanceStatic(const Blas& blas, const glm::mat4& transform);

        /// @brief Clear all instances
        /// @remark TLAS will be marked Dirty!
        void ClearBlasInstances();

        FORAY_GETTER_CR(AnimatedBlasInstances)
        FORAY_GETTER_CR(StaticBlasInstances)

      protected:
        core::Context*                   mContext               = nullptr;
        bool                             mDirty                 = false;
        VkAccelerationStructureKHR       mAccelerationStructure = nullptr;
        core::ManagedBuffer              mTlasMemory;
        util::DualBuffer                 mInstanceBuffer;
        core::ManagedBuffer              mScratchBuffer;
        VkDeviceAddress                  mTlasAddress = 0;
        std::map<uint64_t, BlasInstance> mAnimatedBlasInstances;
        std::map<uint64_t, BlasInstance> mStaticBlasInstances;
        uint64_t                         mNextId = 0;
        GeometryMetaBuffer               mMetaBuffer;
    };
}  // namespace foray::as