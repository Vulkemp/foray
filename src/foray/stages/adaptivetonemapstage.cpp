#include "adaptivetonemapstage.hpp"


foray::stages::AdaptiveTonemapStage::AdaptiveTonemapStage(core::Context* context, RenderDomain* domain, core::ManagedImage* input, int32_t resizeOrder)
    : RasterPostProcessBase(context, domain, resizeOrder), mInput(input)
{
    ConfigureReduceImage(domain->GetExtent());
    // Shader
    mShaderKeys.push_back(mContext->ShaderMan->CompileAndLoadShader(FORAY_SHADER_DIR "/tonemapstage/calcLuminance.frag", mFragmentShader));
    CreateSamplers();
    CreateDescriptorSet();
    CreatePipelineLayout();
    CreatePipeline();
    mTonemapStage.New(mContext, mInput, mReduceFinalMipView, resizeOrder + 1);
}

void foray::stages::AdaptiveTonemapStage::ConfigureReduceImage(VkExtent2D inputExtent)
{
    uint32_t size      = std::max(std::bit_floor(inputExtent.width), std::bit_floor(inputExtent.height));
    uint32_t mipLevels = std::countr_zero(size);

    if(!mReduceImage || mReduceImage->GetExtent2D().width != size)
    {
        if(!!mReduceFinalMipView)
        {
            vkDestroyImageView(mContext->VkDevice(), mReduceFinalMipView, nullptr);
        }

        core::ManagedImage::CreateInfo ci(VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                              | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                          VkFormat::VK_FORMAT_R16_SFLOAT, VkExtent2D{size, size}, "AutoExposure Reduce");
        ci.ImageCI.mipLevels = mipLevels;

        mReduceImage.New(mContext, ci);
        mRenderAttachments.SetAttachmentDiscarded(0, mReduceImage.Get(), VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkImageViewCreateInfo finalMipViewCi         = ci.ImageViewCI;
        finalMipViewCi.image                         = mReduceImage->GetImage();
        finalMipViewCi.subresourceRange.baseMipLevel = mipLevels - 1;
        AssertVkResult(vkCreateImageView(mContext->VkDevice(), &finalMipViewCi, nullptr, &mReduceFinalMipView));
        if(mTonemapStage)
        {
            mTonemapStage->SetAutoExposureImage(mReduceFinalMipView);
        }
    }
}

void foray::stages::AdaptiveTonemapStage::CreateSamplers()
{
    VkSamplerCreateInfo samplerCi{
        .sType        = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter    = VkFilter::VK_FILTER_LINEAR,
        .minFilter    = VkFilter::VK_FILTER_LINEAR,
        .mipmapMode   = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .maxLod       = 1,
        .borderColor  = VkBorderColor::VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
    };
    mInputSampled.Init(mContext, mInput, samplerCi);
}

void foray::stages::AdaptiveTonemapStage::CreateDescriptorSet()
{
    mDescriptorSet.SetDescriptorAt(0, &mInputSampled, VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);
    mDescriptorSet.CreateOrUpdate(mContext, "AutoExposure");
}

void foray::stages::AdaptiveTonemapStage::CreatePipelineLayout()
{
    util::PipelineLayout::Builder builder;
    builder.AddDescriptorSetLayout(mDescriptorSet.GetLayout());
    mPipelineLayout.New(mContext, builder);
}

void foray::stages::AdaptiveTonemapStage::CreatePipeline()
{
    mPipelineBuilder.Default_PostProcess(mVertexShader->GetShaderStageCi(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT),
                                         mFragmentShader->GetShaderStageCi(VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT), &mRenderAttachments, mReduceImage->GetExtent2D(),
                                         mPipelineLayout->GetPipelineLayout());
    mPipeline.New(mContext, mPipelineBuilder);
}

void foray::stages::AdaptiveTonemapStage::ReloadShaders()
{
    mShaderKeys.push_back(mContext->ShaderMan->CompileAndLoadShader(FORAY_SHADER_DIR "/tonemapstage/calcLuminance.frag", mFragmentShader));
    RasterPostProcessBase::ReloadShaders();
    CreatePipeline();
}


void foray::stages::AdaptiveTonemapStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
{
    {  // Prepare input for sampling
        core::ImageLayoutCache::Barrier2 barrier{
            .SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .SrcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
            .DstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .DstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        renderInfo.GetImageLayoutCache().CmdBarrier(cmdBuffer, mInput, barrier, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT);
    }
    {
        VkExtent2D extent{mReduceImage->GetExtent2D()};

        mRenderAttachments.CmdBeginRendering(cmdBuffer, extent, renderInfo.GetImageLayoutCache());

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

        RasterPostProcessBase::CmdDraw(cmdBuffer);

        vkCmdEndRendering(cmdBuffer);
    }
    {  // Prepare reduce image for mip reduce
        VkImageMemoryBarrier2 templateBarrier{
            .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask        = VK_PIPELINE_STAGE_2_BLIT_BIT,
            .oldLayout           = renderInfo.GetImageLayoutCache().Get(mReduceImage.Get()),
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = mReduceImage->GetImage(),
        };
        VkImageMemoryBarrier2 barriers[] = {templateBarrier, templateBarrier};

        barriers[0].dstAccessMask    = VK_ACCESS_2_TRANSFER_READ_BIT;
        barriers[0].newLayout        = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barriers[0].subresourceRange = VkImageSubresourceRange{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1};
        barriers[1].dstAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barriers[1].newLayout        = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barriers[1].subresourceRange =
            VkImageSubresourceRange{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 1, .levelCount = VK_REMAINING_MIP_LEVELS, .layerCount = 1};

        VkDependencyInfo depInfo{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                 .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
                                 .imageMemoryBarrierCount = 2u,
                                 .pImageMemoryBarriers    = barriers};
        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
    }
    {  // Mip Map reduce
        VkImageMemoryBarrier2 barrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                      .srcStageMask        = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                      .srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                      .dstStageMask        = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                      .dstAccessMask       = VK_ACCESS_2_TRANSFER_READ_BIT,
                                      .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                      .image               = mReduceImage->GetImage(),
                                      .subresourceRange =
                                          VkImageSubresourceRange{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};
        VkDependencyInfo      depInfo{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                      .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
                                      .imageMemoryBarrierCount = 1u,
                                      .pImageMemoryBarriers    = &barrier};

        uint32_t mipLevels = std::countr_zero(mReduceImage->GetExtent2D().width);
        for(uint32_t srcLevel = 0; srcLevel < mipLevels - 1; srcLevel++)
        {
            uint32_t dstLevel  = srcLevel + 1;
            int32_t  srcOffset = (int32_t)(1u << (mipLevels - srcLevel));
            int32_t  dstOffset = (int32_t)(1u << (mipLevels - dstLevel));

            barrier.subresourceRange.baseMipLevel = srcLevel;
            if(srcLevel > 0)
            {
                vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
            }

            VkImageBlit2 region{.sType          = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                                .srcSubresource = VkImageSubresourceLayers{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = srcLevel, .layerCount = 1},
                                .srcOffsets     = {VkOffset3D{}, VkOffset3D{srcOffset, srcOffset, 1}},
                                .dstSubresource = VkImageSubresourceLayers{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = dstLevel, .layerCount = 1},
                                .dstOffsets     = {VkOffset3D{}, VkOffset3D{dstOffset, dstOffset, 1}}};

            VkBlitImageInfo2 blitImgInfo{
                .sType          = VkStructureType::VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                .srcImage       = mReduceImage->GetImage(),
                .srcImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .dstImage       = mReduceImage->GetImage(),
                .dstImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .regionCount    = 1u,
                .pRegions       = &region,
                .filter         = VkFilter::VK_FILTER_LINEAR,
            };

            vkCmdBlitImage2(cmdBuffer, &blitImgInfo);
        }
    }
    {  // Convert mip reduce final mip level to shader read only
        uint32_t mipLevels = std::countr_zero(mReduceImage->GetExtent2D().width);

        VkImageMemoryBarrier2 barrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                      .srcStageMask        = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                      .srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                      .dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                      .dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
                                      .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                      .image               = mReduceImage->GetImage(),
                                      .subresourceRange    = VkImageSubresourceRange{
                                             .aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = mipLevels - 1, .levelCount = 1, .layerCount = 1}};
        VkDependencyInfo      depInfo{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                      .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
                                      .imageMemoryBarrierCount = 1u,
                                      .pImageMemoryBarriers    = &barrier};
        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
    }
    {  // Tonemap stage
        mTonemapStage->RecordFrame(cmdBuffer, renderInfo);
    }
}

void foray::stages::AdaptiveTonemapStage::OnResized(VkExtent2D extent)
{
    mTonemapStage->OnResized(extent);
}

void foray::stages::AdaptiveTonemapStage::SetResizeOrder(int32_t priority)
{
    RenderStage::SetResizeOrder(priority);
    mTonemapStage->SetResizeOrder(priority + 1);
}
