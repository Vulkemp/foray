#include "foray_pipelinelayout.hpp"
#include "../core/foray_context.hpp"

namespace foray::util {

    PipelineLayout::~PipelineLayout()
    {
        if(!!mPipelineLayout)
        {
            mContext->DispatchTable().destroyPipelineLayout(mPipelineLayout, nullptr);
            mPipelineLayout = nullptr;
        }
    }

    void PipelineLayout::Builder::AddDescriptorSetLayout(VkDescriptorSetLayout layout)
    {
        mDescriptorSetLayouts.push_back(layout);
    }
    void PipelineLayout::Builder::AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& layouts)
    {
        for(VkDescriptorSetLayout layout : layouts)
        {
            mDescriptorSetLayouts.push_back(layout);
        }
    }
    void PipelineLayout::Builder::AddPushConstantRange(VkPushConstantRange range)
    {
        if(range.offset == ~0U)
        {
            range.offset = mPushConstantOffset;
        }
        mPushConstantOffset += range.size;

        mPushConstantRanges.push_back(range);
    }
    void PipelineLayout::Builder::AddPushConstantRanges(const std::vector<VkPushConstantRange>& ranges)
    {
        for(const VkPushConstantRange& range : ranges)
        {
            AddPushConstantRange(range);
        }
    }

    PipelineLayout::PipelineLayout(core::Context* context, const Builder& builder)
     : mContext(context)
    {
        VkPipelineLayoutCreateInfo ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = builder.GetPNext(),
            .flags = builder.GetFlags(),
        };
        if(builder.GetPushConstantRanges().size() > 0)
        {
            ci.pushConstantRangeCount = builder.GetPushConstantRanges().size();
            ci.pPushConstantRanges    = builder.GetPushConstantRanges().data();
        }
        if(builder.GetDescriptorSetLayouts().size() > 0)
        {
            ci.setLayoutCount = builder.GetDescriptorSetLayouts().size();
            ci.pSetLayouts    = builder.GetDescriptorSetLayouts().data();
        }

        AssertVkResult(mContext->DispatchTable().createPipelineLayout(&ci, nullptr, &mPipelineLayout));
    }
}  // namespace foray::util