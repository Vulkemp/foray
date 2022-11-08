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
        mDescriptorSetLayouts.clear();
        mPushConstantRanges.clear();
        mPushConstantOffset = 0U;
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
        if(range.offset == ~0U)
        {
            range.offset = mPushConstantOffset;
        }
        mPushConstantOffset += range.size;

        mPushConstantRanges.push_back(range);
    }
    void PipelineLayout::AddPushConstantRanges(const std::vector<VkPushConstantRange>& ranges)
    {
        for(const VkPushConstantRange& range : ranges)
        {
            AddPushConstantRange(range);
        }
    }

    VkPipelineLayout PipelineLayout::Build(core::Context* context, VkPipelineLayoutCreateFlags flags, void* pNext)
    {
        if(!!mPipelineLayout)
        {
            mContext->VkbDispatchTable->destroyPipelineLayout(mPipelineLayout, nullptr);
            mPipelineLayout = nullptr;
        }

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

        AssertVkResult(mContext->VkbDispatchTable->createPipelineLayout(&ci, nullptr, &mPipelineLayout));

        return mPipelineLayout;
    }

    VkPipelineLayout PipelineLayout::Build(core::Context*                            context,
                                           const std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                                           const std::vector<VkPushConstantRange>&   pushConstantRanges,
                                           VkPipelineLayoutCreateFlags               flags,
                                           void*                                     pNext)
    {
        AddDescriptorSetLayouts(descriptorLayouts);
        AddPushConstantRanges(pushConstantRanges);

        return Build(context, flags, pNext);
    }

}  // namespace foray::util