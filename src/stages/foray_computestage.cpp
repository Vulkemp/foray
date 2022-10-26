#include "foray_computestage.hpp"

namespace foray::stages {
    void ComputeStage::Init(core::Context* context)
    {
        Destroy();
        mContext = context;
        CreateFixedSizeComponents();
        CreateResolutionDependentComponents();
    }

    void ComputeStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        ApiBeforeFrame(cmdBuffer, renderInfo);

        mContext->VkbDispatchTable->cmdBindPipeline(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);

        std::vector<VkDescriptorSet> descriptorSets = mDescriptorSet.GetDescriptorSets();

        mContext->VkbDispatchTable->cmdBindDescriptorSets(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0U, descriptorSets.size(), descriptorSets.data(), 0U, nullptr);

        glm::uvec3 groupSize;
        ApiBeforeDispatch(cmdBuffer, renderInfo, groupSize);

        mContext->VkbDispatchTable->cmdDispatch(cmdBuffer, groupSize.x, groupSize.y, groupSize.z);
    }

    void ComputeStage::CreateFixedSizeComponents()
    {
        // Descriptor Set Layout
        ApiCreateDescriptorSetLayout();

        ApiCreatePipelineLayout();

        ApiInitShader();

        VkPipelineShaderStageCreateInfo shaderStageCi{.sType  = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                      .stage  = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT,
                                                      .module = mShader,
                                                      .pName  = "main"};

        VkComputePipelineCreateInfo pipelineCi
        {
            .sType = VkStructureType::VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = shaderStageCi,
            .layout = mPipelineLayout.GetPipelineLayout(),
        };

        AssertVkResult(mContext->VkbDispatchTable->createComputePipelines(nullptr, 1U, &pipelineCi, nullptr, &mPipeline));
    }
    void ComputeStage::DestroyFixedComponents() 
    {
        if (!!mPipeline)
        {
            mContext->VkbDispatchTable->destroyPipeline(mPipeline, nullptr);
            mPipeline = nullptr;
        }
        mPipelineLayout.Destroy();
        mDescriptorSet.Destroy();
        mShader.Destroy();
    }
    void ComputeStage::CreateResolutionDependentComponents() {}
    void ComputeStage::DestroyResolutionDependentComponents() {}
}  // namespace foray::stages