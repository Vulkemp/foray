#pragma once
#include "../../util/foray_dualbuffer.hpp"
#include "../foray_component.hpp"

namespace foray::scene {

    struct DrawOp
    {
      public:
        uint64_t                   Order           = 0;
        Mesh*                      Target          = nullptr;
        std::vector<MeshInstance*> Instances       = {};
        uint32_t                   TransformOffset = 0;
    };

    class DrawDirector : public GlobalComponent, public Component::UpdateCallback, public Component::DrawCallback
    {
      public:
        inline DrawDirector() {}

        void InitOrUpdate();

        virtual int32_t GetOrder() const override { return ORDER_TRANSFORM; }

        void CreateBuffers(size_t transformCount);
        void DestroyBuffers();

        virtual void Update(SceneUpdateInfo&) override;
        virtual void Draw(SceneDrawInfo&) override;

        FORAY_PROPERTY_ALLGET(CurrentTransformBuffer)
        FORAY_PROPERTY_ALLGET(PreviousTransformBuffer)

        inline VkDescriptorBufferInfo GetCurrentTransformsDescriptorInfo() const { return mCurrentTransformBuffer.GetDeviceBuffer().GetVkDescriptorBufferInfo(); }
        inline VkDescriptorBufferInfo GetPreviousTransformsDescriptorInfo() const { return mPreviousTransformBuffer.GetVkDescriptorBufferInfo(); }
        inline VkBuffer GetCurrentTransformsVkBuffer() const { return mCurrentTransformBuffer.GetDeviceBuffer().GetBuffer(); }
        inline VkBuffer GetPreviousTransformsVkBuffer() const { return mPreviousTransformBuffer.GetBuffer(); }


      protected:
        util::DualBuffer    mCurrentTransformBuffer;
        core::ManagedBuffer mPreviousTransformBuffer;

        /// @brief Draw Op structs store draw operation
        std::vector<DrawOp> mDrawOps    = {};
        bool                mFirstSetup = true;
        GeometryStore*      mGeo        = nullptr;
        uint32_t            mTotalCount = 0;
    };
}  // namespace foray::scene