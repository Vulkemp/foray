#pragma once
#include "../glTF/hsk_scene.hpp"
#include "../base/hsk_vkcontext.hpp"
#include "hsk_rasterizedRenderStage.hpp"

namespace hsk {
    class GBufferStage : public RasterizedRenderStage
    {
      public:
        ManagedImage* mPositionAttachment = nullptr;
        ManagedImage* mNormalAttachment   = nullptr;
        ManagedImage* mAlbedoAttachment   = nullptr;
        ManagedImage* mMotionAttachment   = nullptr;
        ManagedImage* mMeshIdAttachment   = nullptr;
        ManagedImage* mDepthAttachment    = nullptr;

        VkDescriptorSet mDescriptorSetAttachments = nullptr;
        VkDescriptorSet mDescriptorSetScene       = nullptr;

        GBufferStage() = default;
        virtual ~GBufferStage() { Destroy(); }

        virtual void Init(const VkContext* context, Scene* scene);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;
        virtual void Destroy();

      protected:
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