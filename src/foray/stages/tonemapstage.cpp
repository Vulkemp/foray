#include "tonemapstage.hpp"
#include <imgui/imgui.h>

namespace foray::stages {
    TonemapStage::TonemapStage(foray::core::Context* context, foray::core::ManagedImage* input, int32_t resizeOrder)
        : RasterPostProcessBase(context, context->WindowSwapchain, resizeOrder), mInput(input)
    {
        // Shader
        mShaderKeys.push_back(mContext->ShaderMan->CompileAndLoadShader(FORAY_SHADER_DIR "/tonemapstage/tonemap.frag", mFragmentShader));
        CreateSampler();
        CreateDescriptorSet();
        CreateRenderpass();
        CreatePipelineLayout();
        CreatePipeline();
    }
    void TonemapStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        core::ImageLayoutCache::Barrier2 barrier{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                 .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                 .DstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                                 .DstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                                                 .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        renderInfo.GetImageLayoutCache().CmdBarrier(cmdBuffer, mInput, barrier, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT);

        VkExtent2D extent{mDomain->GetExtent()};

        mRenderAttachments.CmdBeginRendering(cmdBuffer, extent, renderInfo.GetImageLayoutCache(), renderInfo.GetInFlightFrame()->GetSwapchainImageIndex());

        {  // Bind pipeline
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline->GetPipeline());

            // update dynamic state
            VkRect2D   scissor{{}, extent};
            VkViewport viewport{.x = 0.f, .y = 0.f, .width = (fp32_t)extent.width, .height = (fp32_t)extent.height, .minDepth = 0.f, .maxDepth = 1.f};
            vkCmdSetScissor(cmdBuffer, 0u, 1u, &scissor);
            vkCmdSetViewport(cmdBuffer, 0u, 1u, &viewport);
        }

        // Bind descriptorset
        VkDescriptorSet descriptorSet = mDescriptorSet.GetSet();
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout.GetRef(), 0, 1, &descriptorSet, 0, nullptr);

        vkCmdPushConstants(cmdBuffer, mPipelineLayout->GetPipelineLayout(), VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushC), &mPushC);

        RasterPostProcessBase::CmdDraw(cmdBuffer);

        vkCmdEndRendering(cmdBuffer);
    }

    void TonemapStage::OnResized(VkExtent2D extent)
    {
        CreateDescriptorSet();
    }

    void TonemapStage::ImguiSetup()
    {
        const char* tonemappers[] = {
            "Passthrough",
            "Aces",
            "AMD",
        };
        int current = (int)mPushC.TonemapperIdx;
        if(ImGui::Combo("Tonemapper", &current, tonemappers, (int32_t)std::size(tonemappers)))
        {
            mPushC.TonemapperIdx = std::clamp<uint32_t>((uint32_t)current, 0u, 2u);
        }
        ImGui::SliderFloat("Exposure", &mPushC.Exposure, 0.1, 15);
    }

    void TonemapStage::SetMode(EMode mode)
    {
        mPushC.TonemapperIdx = (uint32_t)mode;
    }

    TonemapStage::EMode TonemapStage::GetMode() const
    {
        return (EMode)mPushC.TonemapperIdx;
    }

    void TonemapStage::SetExposure(fp32_t exposure)
    {
        mPushC.Exposure = exposure;
    }

    fp32_t TonemapStage::GetExposure() const
    {
        return mPushC.Exposure;
    }

    void TonemapStage::CreateSampler()
    {
        VkSamplerCreateInfo samplerCi{
            .sType        = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter    = VkFilter::VK_FILTER_NEAREST,
            .minFilter    = VkFilter::VK_FILTER_NEAREST,
            .mipmapMode   = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .addressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .maxLod       = 1,
            .borderColor  = VkBorderColor::VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        };
        mInputSampled.Init(mContext, mInput, samplerCi);
    }

    void TonemapStage::CreateDescriptorSet()
    {
        mDescriptorSet.SetDescriptorAt(0, &mInputSampled, VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);
        mDescriptorSet.CreateOrUpdate(mContext, "Tonemap");
    }
    void TonemapStage::CreateRenderpass()
    {
        mRenderAttachments.AddAttachmentDiscarded(mContext->WindowSwapchain, VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }
    void TonemapStage::CreatePipelineLayout()
    {
        util::PipelineLayout::Builder builder;
        builder.AddDescriptorSetLayout(mDescriptorSet.GetLayout());
        builder.AddPushConstantRange<PushC>(VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);
        mPipelineLayout.New(mContext, builder);
    }
    void TonemapStage::CreatePipeline()
    {
        mPipelineBuilder = util::RasterPipeline::Builder();
        RasterPostProcessBase::ConfigurePipelineBuilder();
        mPipeline.New(mContext, mPipelineBuilder);
    }
    void TonemapStage::ReloadShaders()
    {
        mShaderKeys.push_back(mContext->ShaderMan->CompileAndLoadShader(FORAY_SHADER_DIR "/tonemapstage/tonemap.frag", mFragmentShader));
        RasterPostProcessBase::ReloadShaders();
        CreatePipeline();
    }
}  // namespace foray::stages
