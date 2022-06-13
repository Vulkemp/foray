#pragma once

#include "../hsk_basics.hpp"
#include <vector>
#include <vulkan/vulkan.h>
#include "../glTF/hsk_geo.hpp"

namespace hsk {

    /// @brief Class to quickly build a default pipeline. Ownership is transfered to caller.
    class PipelineBuilder
    {
      public:
        PipelineBuilder()        = default;
        ~PipelineBuilder()       = default;
        PipelineBuilder& operator=(const PipelineBuilder&) = delete;
        PipelineBuilder(const PipelineBuilder&)            = delete;

        VkPipeline       Build();

        HSK_PROPERTY_SET(Context)
        HSK_PROPERTY_SET(RenderPass)
        HSK_PROPERTY_SET(CullMode)
        HSK_PROPERTY_SET(DynamicStates)
        HSK_PROPERTY_SET(PipelineLayout)
        HSK_PROPERTY_SET(DepthTestEnable)
        HSK_PROPERTY_SET(DepthWriteEnable)
        HSK_PROPERTY_SET(ColorBlendAttachmentStates)
        HSK_PROPERTY_SET(SampleCountFlags)
        HSK_PROPERTY_SET(VertexInputStateBuilder)
        HSK_PROPERTY_SET(PrimitiveTopology)
        HSK_PROPERTY_SET(ShaderStageCreateInfos)
        HSK_PROPERTY_SET(PipelineCache)
        HSK_PROPERTY_SET(ColorAttachmentBlendCount)

      protected:
        const VkContext* mContext{};

        VkPipelineLayout mPipelineLayout{};

        VkPrimitiveTopology                               mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkRenderPass                                      mRenderPass{};
        std::vector<VkPipelineShaderStageCreateInfo>*     mShaderStageCreateInfos{};
        VkCullModeFlags                                   mCullMode{VK_CULL_MODE_BACK_BIT};
        const std::vector<VkDynamicState>*                mDynamicStates{};
        std::vector<VkPipelineColorBlendAttachmentState>* mColorBlendAttachmentStates{};
        VkBool32                                          mDepthTestEnable{VK_TRUE};
        VkBool32                                          mDepthWriteEnable{VK_TRUE};
        VkSampleCountFlags                                mSampleCountFlags{0};
        VertexInputStateBuilder*                          mVertexInputStateBuilder{};
        VkPipelineCache                                   mPipelineCache{};
        uint32_t                                          mColorAttachmentBlendCount{0};
    };

}  // namespace hsk