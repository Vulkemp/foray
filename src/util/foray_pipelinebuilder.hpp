#pragma once

#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include "../scene/foray_geo.hpp"
#include <vector>

namespace foray::util {

    /// @brief Class to quickly build a default pipeline. Ownership is transfered to caller.
    class PipelineBuilder
    {
      public:
        PipelineBuilder()                                  = default;
        ~PipelineBuilder()                                 = default;
        PipelineBuilder& operator=(const PipelineBuilder&) = delete;
        PipelineBuilder(const PipelineBuilder&)            = delete;

        VkPipeline Build();

        FORAY_PROPERTY_SET(Context)
        FORAY_PROPERTY_SET(RenderPass)
        FORAY_PROPERTY_SET(CullMode)
        FORAY_PROPERTY_SET(DynamicStates)
        FORAY_PROPERTY_SET(PipelineLayout)
        FORAY_PROPERTY_SET(DepthTestEnable)
        FORAY_PROPERTY_SET(DepthWriteEnable)
        FORAY_PROPERTY_SET(ColorBlendAttachmentStates)
        FORAY_PROPERTY_SET(SampleCountFlags)
        FORAY_PROPERTY_SET(VertexInputStateBuilder)
        FORAY_PROPERTY_SET(PrimitiveTopology)
        FORAY_PROPERTY_SET(ShaderStageCreateInfos)
        FORAY_PROPERTY_SET(PipelineCache)
        FORAY_PROPERTY_SET(ColorAttachmentBlendCount)

      protected:
        const core::VkContext* mContext{};

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
        scene::VertexInputStateBuilder*                   mVertexInputStateBuilder{};
        VkPipelineCache                                   mPipelineCache{};
        uint32_t                                          mColorAttachmentBlendCount{0};
    };

}  // namespace foray::util