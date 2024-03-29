#pragma once

#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include "../scene/foray_geo.hpp"
#include <vector>

namespace foray::util {

    /// @brief Class to quickly build a default pipeline. Ownership is transfered to caller.
	/// TODO: THIS CLASS SHOULD BE REWORKED
	/// => all properties of the pipeline should be configurable, for example by exposing the underlying structs directly, instead of individual
	/// access methods
    class PipelineBuilder
    {
      public:
        PipelineBuilder()                                  = default;
        ~PipelineBuilder()                                 = default;
        PipelineBuilder& operator=(const PipelineBuilder&) = delete;
        PipelineBuilder(const PipelineBuilder&)            = delete;

        VkPipeline Build();

        FORAY_PROPERTY_V(Context)
        FORAY_PROPERTY_V(RenderPass)
        FORAY_PROPERTY_V(CullMode)
        FORAY_PROPERTY_V(PolygonMode)
        FORAY_PROPERTY_V(DynamicStates)
        FORAY_PROPERTY_V(PipelineLayout)
        FORAY_PROPERTY_V(DepthTestEnable)
        FORAY_PROPERTY_V(DepthWriteEnable)
        FORAY_PROPERTY_V(ColorBlendAttachmentStates)
        FORAY_PROPERTY_V(SampleCountFlags)
        FORAY_PROPERTY_V(VertexInputStateBuilder)
        FORAY_PROPERTY_V(PrimitiveTopology)
        FORAY_PROPERTY_V(ShaderStageCreateInfos)
        FORAY_PROPERTY_V(PipelineCache)
        FORAY_PROPERTY_V(ColorAttachmentBlendCount)

      protected:
        core::Context* mContext{};

        VkPipelineLayout mPipelineLayout{};

        VkPrimitiveTopology                               mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkRenderPass                                      mRenderPass{};
        std::vector<VkPipelineShaderStageCreateInfo>*     mShaderStageCreateInfos{};
        VkCullModeFlags                                   mCullMode{VK_CULL_MODE_BACK_BIT};
        VkPolygonMode                                     mPolygonMode{VK_POLYGON_MODE_FILL};
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