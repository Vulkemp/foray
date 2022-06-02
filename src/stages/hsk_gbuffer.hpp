#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "../glTF/hsk_scene.hpp"
#include "hsk_rasterizedRenderStage.hpp"

namespace hsk {
    class GBufferStage : public RasterizedRenderStage
    {
      public:
        ManagedImage mPositionAttachment;
        ManagedImage mNormalAttachment;
        ManagedImage mAlbedoAttachment;
        ManagedImage mMotionAttachment;
        ManagedImage mMeshIdAttachment;
        ManagedImage mDepthAttachment;

        GBufferStage() = default;
        virtual ~GBufferStage() { Destroy(); }

        virtual void Init(const VkContext* context, Scene* scene);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;
        virtual void Destroy();

      protected:
        const uint32_t mAttachmentCountColor = 5;
        const uint32_t mAttachmentCountDepth = 1;

        Scene* mScene;

        virtual void InitFixedSizeComponents();
        virtual void InitResolutionDependentComponents();
        virtual void DestroyResolutionDependentComponents();

        void PrepareAttachments();
        void PrepareRenderpass();
        void SetupDescriptors();
        void BuildCommandBuffer(){};
        void PreparePipeline();
    };
}  // namespace hsk