#include "rasterpostprocess.hpp"

namespace foray::stages
{
    RasterPostProcessBase::RasterPostProcessBase(core::Context* context, RenderDomain* domain, uint32_t resizeOrder) 
     : RenderStage(context, domain, resizeOrder)
    {
        mShaderKeys.push_back(mContext->ShaderMan->CompileAndLoadShader(FORAY_SHADER_DIR "/rasterpostprocessstage/rasterpp.vert", mVertexShader));
    }

    void RasterPostProcessBase::ReloadShaders() 
    {
        mShaderKeys.push_back(mContext->ShaderMan->CompileAndLoadShader(FORAY_SHADER_DIR "/rasterpostprocessstage/rasterpp.vert", mVertexShader));
        mPipeline.New(mContext, mPipelineBuilder);
    }

    void RasterPostProcessBase::CmdDraw(VkCommandBuffer cmdBuffer) 
    {
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
    }

    void RasterPostProcessBase::ConfigurePipelineBuilder() 
    {
        mPipelineBuilder.Default_PostProcess(mVertexShader->GetShaderStageCi(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT), mFragmentShader->GetShaderStageCi(VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT), 
        &mRenderAttachments, mDomain->GetExtent(), mPipelineLayout->GetPipelineLayout());
    }


} // namespace foray::stages
