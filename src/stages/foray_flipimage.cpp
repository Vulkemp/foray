#include "foray_flipimage.hpp"
#include "../core/foray_shadermodule.hpp"
#include "../util/foray_pipelinebuilder.hpp"
// #include "../utility/foray_shaderstagecreateinfos.hpp"


namespace foray::stages {
    void FlipImageStage::Init(const core::VkContext* context, core::ManagedImage* sourceImage)
    {
        mContext     = context;
        mSourceImage = sourceImage;

        CreateResolutionDependentComponents();
        CreateFixedSizeComponents();

        // Clear values for all attachments written in the fragment shader
        mClearValues.resize(mColorAttachments.size() + 1);
        for(size_t i = 0; i < mColorAttachments.size(); i++)
        {
            mClearValues[i].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        }
        mClearValues[mColorAttachments.size()].depthStencil = {1.0f, 0};
    }

    void FlipImageStage::CreateFixedSizeComponents() {}

    void FlipImageStage::DestroyFixedComponents()
    {
        VkDevice device = mContext->Device;
        if(mPipeline)
        {
            vkDestroyPipeline(device, mPipeline, nullptr);
            mPipeline = nullptr;
        }
        if(mPipelineLayout)
        {
            vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
            mPipelineLayout = nullptr;
        }
        if(mPipelineCache)
        {
            vkDestroyPipelineCache(device, mPipelineCache, nullptr);
            mPipelineCache = nullptr;
        }
        mDescriptorSet.Destroy();
    }

    void FlipImageStage::CreateResolutionDependentComponents()
    {
        PrepareAttachments();
    }

    void FlipImageStage::DestroyResolutionDependentComponents()
    {
        VkDevice device = mContext->Device;
        for(auto& colorAttachment : mColorAttachments)
        {
            colorAttachment->Destroy();
        }
        if(mFrameBuffer)
        {
            vkDestroyFramebuffer(device, mFrameBuffer, nullptr);
            mFrameBuffer = nullptr;
        }
        if(mRenderpass)
        {
            vkDestroyRenderPass(device, mRenderpass, nullptr);
            mRenderpass = nullptr;
        }
    }

    void FlipImageStage::PrepareAttachments()
    {
        static const VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

        static const VkImageUsageFlags imageUsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkExtent3D               extent                = {mContext->Swapchain.extent.width, mContext->Swapchain.extent.height, 1};
        VmaMemoryUsage           memoryUsage           = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VmaAllocationCreateFlags allocationCreateFlags = 0;
        VkImageLayout            intialLayout          = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageAspectFlags       aspectMask            = VK_IMAGE_ASPECT_COLOR_BIT;

        mFlipImages.clear();
        mFlipImages.reserve(5);
        mFlipImages.push_back(std::make_unique<core::ManagedImage>());
        mFlipImages[0]->Create(mContext, memoryUsage, allocationCreateFlags, extent, imageUsageFlags, colorFormat, intialLayout, aspectMask, FlipTarget);


        mColorAttachments.clear();
        mColorAttachments.reserve(mFlipImages.size());
        for(size_t i = 0; i < mFlipImages.size(); i++)
        {
            mColorAttachments.push_back(mFlipImages[i].get());
        }
    }


    void FlipImageStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        VkImageSubresourceRange range{};
        range.aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        VkImageMemoryBarrier barrier{};
        barrier.sType            = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.subresourceRange = range;

        // transition source attachment to use as transfer source
        core::ManagedImage::LayoutTransitionInfo layoutTransitionInfo;
        layoutTransitionInfo.CommandBuffer        = cmdBuffer;
        layoutTransitionInfo.BarrierSrcAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // wait for color attachment to be written
        layoutTransitionInfo.BarrierDstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;           // block transfer
        layoutTransitionInfo.OldImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
        layoutTransitionInfo.NewImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        layoutTransitionInfo.SrcQueueFamilyIndex  = mContext->QueueGraphics;
        layoutTransitionInfo.DstQueueFamilyIndex  = mContext->QueueGraphics;
        layoutTransitionInfo.SubresourceRange     = range;
        layoutTransitionInfo.SrcStage =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;               // wait for color output (although this is prob correctly synchronized by previous renderpass?)
        layoutTransitionInfo.DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;  // block transfer
        mSourceImage->TransitionLayout(layoutTransitionInfo);

        // transition target attachment
        layoutTransitionInfo.SrcStage             = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;               //wait for nothing
        layoutTransitionInfo.DstStage             = VK_PIPELINE_STAGE_TRANSFER_BIT;                  // block transfer
        layoutTransitionInfo.BarrierSrcAccessMask = 0;                                               // we don't have to wait for anything here.
        layoutTransitionInfo.BarrierDstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;  // block the transfer write
        layoutTransitionInfo.OldImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        layoutTransitionInfo.NewImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        mFlipImages[0]->TransitionLayout(layoutTransitionInfo);

        VkImageSubresourceLayers layers = {};
        layers.aspectMask               = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        layers.mipLevel                 = 0;
        layers.baseArrayLayer           = 0;
        layers.layerCount               = 1;

        VkImageBlit blitRegion    = {};
        blitRegion.srcSubresource = layers;
        blitRegion.srcOffsets[0]  = {};
        blitRegion.srcOffsets[1]  = VkOffset3D{.x = (int32_t)mContext->Swapchain.extent.width, .y = (int32_t)mContext->Swapchain.extent.height, .z = 1};
        blitRegion.dstSubresource = layers;
        blitRegion.dstOffsets[1]  = {.z = 1};
        blitRegion.dstOffsets[0]  = VkOffset3D{.x = (int32_t)mContext->Swapchain.extent.width, .y = (int32_t)mContext->Swapchain.extent.height, .z = 0};

        vkCmdBlitImage(cmdBuffer, mSourceImage->GetImage(), VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mFlipImages[0]->GetImage(),
                       VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VkFilter::VK_FILTER_NEAREST);

        // transition source attachment back

        layoutTransitionInfo.SrcStage             = VK_PIPELINE_STAGE_TRANSFER_BIT;                 // wait for transfer read to be done
        layoutTransitionInfo.DstStage             = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // block writing to this image again, before layout transition is done
        layoutTransitionInfo.BarrierSrcAccessMask = 0;                                              // no memory needs to be flushed after reading from this image
        layoutTransitionInfo.BarrierDstAccessMask = 0;                                              // no memory has to be flushed after this layout transition
        layoutTransitionInfo.OldImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        layoutTransitionInfo.NewImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
        mSourceImage->TransitionLayout(layoutTransitionInfo);

        layoutTransitionInfo.SrcStage             = VK_PIPELINE_STAGE_TRANSFER_BIT;                          // wait for transfer write
        layoutTransitionInfo.DstStage             = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;           // block writing to this image again
        layoutTransitionInfo.BarrierSrcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;          // flush memory after write
        layoutTransitionInfo.BarrierDstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // wait with color attachment writing on this image
        layoutTransitionInfo.OldImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        layoutTransitionInfo.NewImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
        mFlipImages[0]->TransitionLayout(layoutTransitionInfo);
    }

    void FlipImageStage::OnResized(const VkExtent2D& extent, core::ManagedImage* newSourceImage)
    {
        mSourceImage = newSourceImage;
        RasterizedRenderStage::OnResized(extent);
    }


}  // namespace foray::stages