#include "computestage.hpp"

namespace foray::stages {
    ComputeStageBase::ComputeStageBase(core::Context* context, RenderDomain* domain, uint32_t resizeOrder)
     : RenderStage(context, domain, resizeOrder)
    {
    }

    void ComputeStageBase::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        ApiBeforeFrame(cmdBuffer, renderInfo);

        mContext->DispatchTable().cmdBindPipeline(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);

        glm::uvec3 groupSize;
        ApiBeforeDispatch(cmdBuffer, renderInfo, groupSize);

        mContext->DispatchTable().cmdDispatch(cmdBuffer, groupSize.x, groupSize.y, groupSize.z);
    }

    void ComputeStageBase::CreatePipeline()
    {
        VkPipelineShaderStageCreateInfo shaderStageCi{.sType  = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                      .stage  = vk::ShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT,
                                                      .module = mShader.GetRef(),
                                                      .pName  = "main"};

        VkComputePipelineCreateInfo pipelineCi
        {
            .sType = VkStructureType::VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = shaderStageCi,
            .layout = mPipelineLayout.GetRef(),
        };

        AssertVkResult(mContext->DispatchTable().createComputePipelines(nullptr, 1U, &pipelineCi, nullptr, &mPipeline));
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