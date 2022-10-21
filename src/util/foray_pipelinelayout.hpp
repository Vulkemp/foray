#pragma once

#include "../foray_basics.hpp"
#include <map>
#include <vector>
#include "../foray_vulkan.hpp"
#include "../core/foray_context.hpp"

namespace foray::util {
    /// @brief Class that holds memory ownership of a vulkan pipeline
    class PipelineLayout
    {
        PipelineLayout()        = default;
        PipelineLayout& operator=(const PipelineLayout&) = delete;
        PipelineLayout(const PipelineLayout&)             = delete;
        ~PipelineLayout()
        {
            if(mPipelineLayout)
                vkDestroyPipelineLayout(mContext->Device(), mPipelineLayout, nullptr);
        }

        void Create(core::Context*                    context,
                    std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                    std::vector<VkPushConstantRange>*   pushConstantRanges = nullptr,
                    VkPipelineLayoutCreateFlags         flags              = 0,
                    void*                               pNext              = nullptr)
        {
            mContext = context;

            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
            pipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.flags                      = flags;
            pipelineLayoutCreateInfo.pNext                      = pNext;
            pipelineLayoutCreateInfo.pushConstantRangeCount     = pushConstantRanges != nullptr ? pushConstantRanges->size() : 0;
            pipelineLayoutCreateInfo.pPushConstantRanges        = pushConstantRanges != nullptr ? pushConstantRanges->data() : nullptr;
            pipelineLayoutCreateInfo.setLayoutCount             = descriptorLayouts.size();
            pipelineLayoutCreateInfo.pSetLayouts                = descriptorLayouts.data();

            AssertVkResult(vkCreatePipelineLayout(mContext->Device(), &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout));
        }

        FORAY_PROPERTY_GET(PipelineLayout)

      protected:
        core::Context* mContext{};
        VkPipelineLayout mPipelineLayout{};
    };
}  // namespace foray