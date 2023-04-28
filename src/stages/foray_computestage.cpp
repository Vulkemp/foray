#include "foray_computestage.hpp"

namespace foray::stages {
    ComputeStageBase::ComputeStageBase(core::Context* context)
     : RenderStage(context)
    {
        ApiCreateDescriptorSet();
        ApiCreatePipelineLayout();
        ApiInitShader();
        CreatePipeline();
    }

    void ComputeStageBase::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        ApiBeforeFrame(cmdBuffer, renderInfo);

        mContext->DispatchTable().cmdBindPipeline(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);

        VkDescriptorSet descriptorSet = mDescriptorSet.GetDescriptorSet();

        mContext->DispatchTable().cmdBindDescriptorSets(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0U, 1U, &descriptorSet, 0U, nullptr);

        glm::uvec3 groupSize;
        ApiBeforeDispatch(cmdBuffer, renderInfo, groupSize);

        mContext->DispatchTable().cmdDispatch(cmdBuffer, groupSize.x, groupSize.y, groupSize.z);
    }

    void ComputeStageBase::CreatePipeline()
    {
        VkPipelineShaderStageCreateInfo shaderStageCi{.sType  = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                      .stage  = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT,
                                                      .module = *mShader.Get(),
                                                      .pName  = "main"};

        VkComputePipelineCreateInfo pipelineCi
        {
            .sType = VkStructureType::VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = shaderStageCi,
            .layout = mPipelineLayout,
        };

        AssertVkResult(mContext->DispatchTable().createComputePipelines(nullptr, 1U, &pipelineCi, nullptr, &mPipeline));
    }
    void ComputeStageBase::ReloadShaders()
    {
        if (!!mPipeline)
        {
            mContext->DispatchTable().destroyPipeline(mPipeline, nullptr);
            mPipeline = nullptr;
            mShader = nullptr;
            ApiInitShader();
            CreatePipeline();
        }
    } 
    ComputeStageBase::~ComputeStageBase() 
    {
        if (!!mPipeline)
        {
            mContext->DispatchTable().destroyPipeline(mPipeline, nullptr);
            mPipeline = nullptr;
        }
    }
}  // namespace foray::stages