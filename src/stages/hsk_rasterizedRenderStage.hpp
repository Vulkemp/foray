#pragma once
#include "../vulkan/hsk_intermediateBuffer.hpp"
#include "hsk_renderstage.hpp"

namespace hsk {
    class RasterizedRenderStage : public RenderStage
    {
        HSK_PROPERTY_CGET(FrameBuffer)
        HSK_PROPERTY_CGET(DescriptorPool)
        HSK_PROPERTY_CGET(DescriptorSetLayout)
        HSK_PROPERTY_CGET(PipelineCache)
        HSK_PROPERTY_CGET(Renderpass)
        HSK_PROPERTY_CGET(Pipeline)
        HSK_PROPERTY_CGET(PipelineLayout)

      protected:

        VkFramebuffer         mFrameBuffer         = nullptr;
        VkDescriptorPool      mDescriptorPool      = nullptr;
        VkDescriptorSetLayout mDescriptorSetLayout = nullptr;
        VkPipelineCache       mPipelineCache       = nullptr;
        VkRenderPass          mRenderpass          = nullptr;
        VkPipeline            mPipeline            = nullptr;
        VkPipelineLayout      mPipelineLayout      = nullptr;

        virtual void InitDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);
        virtual void InitDescriptorSetLayout();

        virtual void Destroy() override;
        virtual void DestroyFixedComponents();
        virtual void DestroyResolutionDependentComponents();
    };
}  // namespace hsk
