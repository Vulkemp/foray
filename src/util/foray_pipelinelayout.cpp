#include "foray_pipelinelayout.hpp"
#include "../core/foray_context.hpp"

namespace foray::util {

    void PipelineLayout::Destroy()
    {
        if(!!mPipelineLayout)
        {
            mContext->VkbDispatchTable->destroyPipelineLayout(mPipelineLayout, nullptr);
            mPipelineLayout = nullptr;
        }
    }

    void PipelineLayout::AddDescriptorSetLayout(VkDescriptorSetLayout layout)
    {
        mDescriptorSetLayouts.push_back(layout);
    }
    void PipelineLayout::AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& layouts)
    {
        for(VkDescriptorSetLayout layout : layouts)
        {
            mDescriptorSetLayouts.push_back(layout);
        }
    }
    void PipelineLayout::AddPushConstantRange(VkPushConstantRange range)
    {
        mPushConstantRanges.push_back(range);
    }
    void PipelineLayout::AddPushConstantRanges(const std::vector<VkPushConstantRange>& ranges)
    {
        for(VkPushConstantRange range : ranges)
        {
            mPushConstantRanges.push_back(range);
        }
    }

    VkPipelineLayout PipelineLayout::Create(core::Context* context, VkPipelineLayoutCreateFlags flags, void* pNext)
    {
        Destroy();
        mContext = context;

        VkPipelineLayoutCreateInfo ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = pNext,
            .flags = flags,
        };
        if(mPushConstantRanges.size() > 0)
        {
            ci.pushConstantRangeCount = mPushConstantRanges.size();
            ci.pPushConstantRanges    = mPushConstantRanges.data();
        }
        if(mDescriptorSetLayouts.size() > 0)
        {
            ci.setLayoutCount = mDescriptorSetLayouts.size();
            ci.pSetLayouts    = mDescriptorSetLayouts.data();
        }

        AssertVkResult(vkCreatePipelineLayout(mContext->Device(), &ci, nullptr, &mPipelineLayout));

        return mPipelineLayout;
    }

    VkPipelineLayout PipelineLayout::Create(core::Context*                            context,
                                            const std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                                            const std::vector<VkPushConstantRange>&   pushConstantRanges,
                                            VkPipelineLayoutCreateFlags               flags,
                                            void*                                     pNext)
    {
        AddDescriptorSetLayouts(descriptorLayouts);
        AddPushConstantRanges(pushConstantRanges);

        return Create(context, flags, pNext);
    }

}  // namespace foray::util