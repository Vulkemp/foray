#pragma once
#include "../hsk_basics.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include "../scenegraph/hsk_component.hpp"
#include <map>
#include <vulkan/vulkan.h>

namespace hsk {

    class Blas;

    class BlasInstance
    {
      public:
        inline BlasInstance() {}
        explicit BlasInstance(const VkContext* context, MeshInstance* meshInstance);

        HSK_PROPERTY_ALLGET(Blas)
        HSK_PROPERTY_ALLGET(MeshInstance)
        HSK_PROPERTY_ALLGET(Transform)
        HSK_PROPERTY_ALLGET(AsInstance)

        void Update();

      protected:
        const VkContext*                   mContext      = nullptr;
        Blas*                              mBlas         = nullptr;
        MeshInstance*                      mMeshInstance = nullptr;
        Transform*                         mTransform;
        VkAccelerationStructureInstanceKHR mAsInstance = {};
    };

    /// @brief Describes a top level accerlation structure. A tlas usually holds multiple Blas.
    /// A blas is an object/mesh instance together with its position/rotation in the 3d space.
    class Tlas : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        virtual ~Tlas() { Destroy(); }

        virtual void Create();
        virtual void Update(const FrameUpdateInfo& drawInfo);
        virtual void Destroy();

        HSK_PROPERTY_CGET(AccelerationStructure)
        HSK_PROPERTY_CGET(TlasMemory)
        HSK_PROPERTY_CGET(TlasAddress)

        operator VkAccelerationStructureKHR() { return mAccelerationStructure; }

      protected:
        const VkContext*                                       mContext{};
        VkAccelerationStructureKHR                             mAccelerationStructure = nullptr;
        ManagedBuffer                                          mTlasMemory;
        ManagedBuffer                                          mInstanceBuffer;
        ManagedBuffer                                          mScratchBuffer;
        VkDeviceAddress                                        mTlasAddress{};
        std::map<MeshInstance*, std::unique_ptr<BlasInstance>> mAnimatedBlasInstances;
        std::map<MeshInstance*, std::unique_ptr<BlasInstance>> mStaticBlasInstances;
    };
}  // namespace hsk