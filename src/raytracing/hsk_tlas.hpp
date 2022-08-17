#pragma once
#include "../hsk_basics.hpp"
#include "../memory/hsk_dualbuffer.hpp"
#include "../scenegraph/hsk_component.hpp"
#include <map>
#include <vulkan/vulkan.h>

namespace hsk {

    class Blas;

    class BlasInstance
    {
      public:
        using TransformUpdateFunc = std::function<void(glm::mat4&)>;

        inline BlasInstance() {}
        BlasInstance(uint64_t instanceId, uint64_t blasRef, TransformUpdateFunc getUpdatedGlobalTransformFunc);
        BlasInstance(uint64_t instanceId, uint64_t blasRef, const glm::mat4& globalTransform);

        HSK_PROPERTY_CGET(InstanceId)
        HSK_PROPERTY_ALLGET(AsInstance)
        HSK_PROPERTY_ALL(GetUpdatedGlobalTransformFunc)

        bool IsAnimated() const { return !!mGetUpdatedGlobalTransformFunc; }

        void Update();

        static void TranslateTransformMatrix(const glm::mat4& in, VkTransformMatrixKHR& out);

      protected:
        uint64_t                           mInstanceId                    = 0UL;
        TransformUpdateFunc  mGetUpdatedGlobalTransformFunc = nullptr;
        VkAccelerationStructureInstanceKHR mAsInstance                    = {};
    };

    /// @brief Describes a top level accerlation structure. A tlas usually holds multiple Blas.
    /// A blas is an object/mesh instance together with its position/rotation in the 3d space.
    class Tlas
    {
      public:
        explicit Tlas(const VkContext* context);
        virtual ~Tlas() { Destroy(); }

        virtual void CreateOrUpdate();
        virtual void UpdateLean(VkCommandBuffer cmdBuffer, uint32_t frameIndex);
        virtual void Destroy();

        HSK_PROPERTY_CGET(AccelerationStructure)
        HSK_PROPERTY_CGET(TlasMemory)
        HSK_PROPERTY_CGET(TlasAddress)

        operator VkAccelerationStructureKHR() { return mAccelerationStructure; }

        void          RemoveBlasInstance(uint64_t id);
        BlasInstance* GetBlasInstance(uint64_t id);
        uint64_t      AddBlasInstanceAuto(MeshInstance* meshInstance);
        uint64_t      AddBlasInstanceAnimated(const Blas& blas, BlasInstance::TransformUpdateFunc getUpdatedGlobalTransformFunc);
        uint64_t      AddBlasInstanceStatic(const Blas& blas, const glm::mat4& transform);

        void ClearBlasInstances();

        HSK_PROPERTY_ALLGET(AnimatedBlasInstances)
        HSK_PROPERTY_ALLGET(StaticBlasInstances)

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
    };
}  // namespace hsk