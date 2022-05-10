#pragma once
#include "hsk_rasterizedRenderStage.hpp"

namespace hsk {
    class GBufferStage : public RasterizedRenderStage
    {
      public:
        IntermediateBuffer* m_PositionAttachment = nullptr;
        IntermediateBuffer* m_NormalAttachment   = nullptr;
        IntermediateBuffer* m_AlbedoAttachment   = nullptr;
        IntermediateBuffer* m_MotionAttachment   = nullptr;
        IntermediateBuffer* m_MeshIdAttachment   = nullptr;
        IntermediateBuffer* m_DepthAttachment    = nullptr;

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