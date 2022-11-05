#include "foray_computestage.hpp"

namespace foray::stages {
    void ComputeStage::Init(core::Context* context)
    {
        Destroy();
        mContext = context;
        ApiCreateDescriptorSet();
        ApiCreatePipelineLayout();
        ApiInitShader();
        CreatePipeline();
    }

    void ComputeStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        ApiBeforeFrame(cmdBuffer, renderInfo);

        mContext->VkbDispatchTable->cmdBindPipeline(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);

        VkDescriptorSet descriptorSet = mDescriptorSet.GetDescriptorSet();

        mContext->VkbDispatchTable->cmdBindDescriptorSets(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0U, 1U, &descriptorSet, 0U, nullptr);

        glm::uvec3 groupSize;
        ApiBeforeDispatch(cmdBuffer, renderInfo, groupSize);

        mContext->VkbDispatchTable->cmdDispatch(cmdBuffer, groupSize.x, groupSize.y, groupSize.z);
    }

    void ComputeStage::CreatePipeline()
    {
        VkPipelineShaderStageCreateInfo shaderStageCi{.sType  = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                      .stage  = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT,
                                                      .module = mShader,
                                                      .pName  = "main"};

        VkComputePipelineCreateInfo pipelineCi
        {
            .sType = VkStructureType::VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = shaderStageCi,
            .layout = mPipelineLayout,
        };

        AssertVkResult(mContext->VkbDispatchTable->createComputePipelines(nullptr, 1U, &pipelineCi, nullptr, &mPipeline));
    }
    void ComputeStage::Destroy() 
    {
        if (!!mPipeline)
        {
            mContext->VkbDispatchTable->destroyPipeline(mPipeline, nullptr);
            mPipeline = nullptr;
        }
        mShader.Destroy();
        mDescriptorSet.Destroy();
        mPipelineLayout.Destroy();
    }
}  // namespace foray::stages