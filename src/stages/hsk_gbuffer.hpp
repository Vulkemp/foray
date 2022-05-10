#pragma once
#include "hsk_rasterizedRenderStage.hpp"

namespace hsk {
    class GBufferStage : public RasterizedRenderStage
    {
      public:
        IntermediateImage* m_PositionAttachment = nullptr;
        IntermediateImage* m_NormalAttachment   = nullptr;
        IntermediateImage* m_AlbedoAttachment   = nullptr;
        IntermediateImage* m_MotionAttachment   = nullptr;
        IntermediateImage* m_MeshIdAttachment   = nullptr;
        IntermediateImage* m_DepthAttachment    = nullptr;

        VkDescriptorSet  mDescriptorSetAttachments = nullptr;
        VkDescriptorSet  mDescriptorSetScene       = nullptr;

        GBufferStage();
        virtual ~GBufferStage() { Destroy(); }

        virtual void Init();
        virtual void RecordFrame(const VkCommandBuffer*& out_commandBuffers, uint32_t& out_commandBufferCount);
        virtual void Destroy();

      protected:

        virtual void InitFixedSizeComponents();
        virtual void InitResolutionDependentComponents();
        virtual void DestroyResolutionDependentComponents();

        void prepareAttachments();
        void prepareRenderpass();
        void setupDescriptorPool();
        void setupDescriptorSetLayout();
        void setupDescriptorSet();
        void buildCommandBuffer();
        void preparePipeline();
    };
}  // namespace hsk