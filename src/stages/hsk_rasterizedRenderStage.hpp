#pragma once
#include "../memory/hsk_intermediateImage.hpp"
#include "hsk_renderstage.hpp"

namespace hsk {
    class RasterizedRenderStage : public RenderStage
    {
      public:
        RasterizedRenderStage(){};

        HSK_PROPERTY_CGET(FrameBuffer)
        HSK_PROPERTY_CGET(DescriptorPool)
        HSK_PROPERTY_CGET(DescriptorSetLayout)
        HSK_PROPERTY_CGET(PipelineCache)
        HSK_PROPERTY_CGET(Renderpass)
        HSK_PROPERTY_CGET(Pipeline)
        HSK_PROPERTY_CGET(PipelineLayout)

      protected:

        struct BindingInfo
        {
            uint32_t           DescriptorCount{};  // How many descriptors are referenced at the same time, usually 1.
            VkDescriptorType   DescriptorType{};
            VkShaderStageFlags ShaderStageFlags{};
            uint32_t           PoolSizeDescriptorCount{};  // How many descriptors of this exist in total, for example 3 ubos for 3 swapchain images, etc.
            VkSampler*         pImmutableSamplers{nullptr};
        };

        std::vector<BindingInfo> mBindingInfos;

        VkFramebuffer         mFrameBuffer         = nullptr;
        VkDescriptorPool      mDescriptorPool      = nullptr;
        VkDescriptorSetLayout mDescriptorSetLayout = nullptr;
        VkPipelineCache       mPipelineCache       = nullptr;
        VkRenderPass          mRenderpass          = nullptr;
        VkPipeline            mPipeline            = nullptr;
        VkPipelineLayout      mPipelineLayout      = nullptr;

        virtual void InitDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);
        virtual void InitDescriptorSetLayout(){};

        virtual void Destroy() override;
        virtual void DestroyFixedComponents();
        virtual void DestroyResolutionDependentComponents();
    };
}  // namespace hsk
