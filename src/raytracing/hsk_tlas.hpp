#pragma once
#include "../hsk_basics.hpp"
#include "../memory/hsk_dualbuffer.hpp"
#include "../scenegraph/hsk_component.hpp"
#include "hsk_geometrymetabuffer.hpp"
#include "hsk_rt_declares.hpp"
#include "hsk_blasinstance.hpp"
#include <map>
#include <vulkan/vulkan.h>

namespace hsk {


    /// @brief Describes a top level accerlation structure. A tlas usually holds multiple Blas.
    /// A blas is an object/mesh instance together with its transformation in 3d space.
    class Tlas : public DeviceResourceBase
    {
      public:
        explicit Tlas(const VkContext* context);
        virtual ~Tlas() { Destroy(); }

        /// @brief (Re)creates the TLAS. Required to invoke when changes to the TLAS transitioned it to Dirty state. Will synchronize with the CPU.
        virtual void CreateOrUpdate();
        /// @brief Updates transforms only. TLAS rebuild is performed and synchronized on GPU only. TLAS must by non-Dirty!
        virtual void UpdateLean(VkCommandBuffer cmdBuffer, uint32_t frameIndex);

        inline virtual bool Exists() const override { return !!mAccelerationStructure; }
        virtual void        Destroy() override;

        HSK_PROPERTY_CGET(AccelerationStructure)
        HSK_PROPERTY_CGET(TlasMemory)
        HSK_PROPERTY_CGET(TlasAddress)
        HSK_PROPERTY_ALLGET(MetaBuffer)
        HSK_PROPERTY_CGET(Dirty)

        operator VkAccelerationStructureKHR() { return mAccelerationStructure; }

        /// @brief Remove a BLAS instance
        /// @remark TLAS will be marked Dirty!
        void RemoveBlasInstance(uint64_t id);
        /// @brief Get A BLAS instance by id
        const BlasInstance* GetBlasInstance(uint64_t id) const;
        /// @brief Add a BLAS instance from meshInstance component
        /// @remark TLAS will be marked Dirty!
        uint64_t AddBlasInstanceAuto(MeshInstance* meshInstance);
        /// @brief Add an animated BLAS instance
        /// @remark TLAS will be marked Dirty!
        uint64_t AddBlasInstanceAnimated(const Blas& blas, BlasInstance::TransformUpdateFunc getUpdatedGlobalTransformFunc);
        /// @brief Add a static BLAS instance
        /// @remark TLAS will be marked Dirty!
        uint64_t AddBlasInstanceStatic(const Blas& blas, const glm::mat4& transform);

        /// @brief Clear all instances
        /// @remark TLAS will be marked Dirty!
        void ClearBlasInstances();

        HSK_PROPERTY_CGET(AnimatedBlasInstances)
        HSK_PROPERTY_CGET(StaticBlasInstances)

      protected:
        const VkContext*                 mContext               = nullptr;
        bool                             mDirty                 = false;
        VkAccelerationStructureKHR       mAccelerationStructure = nullptr;
        ManagedBuffer                    mTlasMemory;
        DualBuffer                       mInstanceBuffer;
        ManagedBuffer                    mScratchBuffer;
        VkDeviceAddress                  mTlasAddress = 0;
        std::map<uint64_t, BlasInstance> mAnimatedBlasInstances;
        std::map<uint64_t, BlasInstance> mStaticBlasInstances;
        uint64_t                         mNextId = 0;
        GeometryMetaBuffer               mMetaBuffer;
    };
}  // namespace hsk