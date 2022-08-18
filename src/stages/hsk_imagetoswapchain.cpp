#include "hsk_imagetoswapchain.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../memory/hsk_commandbuffer.hpp"


namespace hsk {
    void ImageToSwapchainStage::Init(const VkContext* context, ManagedImage* sourceImage, const PostCopy& postcopy)
    {
        mContext     = context;
        mSourceImage = sourceImage;
        mPostCopy    = postcopy;
    }

    void ImageToSwapchainStage::Init(const VkContext* context, ManagedImage* sourceImage)
    {
        mContext     = context;
        mSourceImage = sourceImage;
    }

    void ImageToSwapchainStage::OnResized(const VkExtent2D& extent, ManagedImage* newSourceImage)
    {
        mSourceImage = newSourceImage;
    }

    void ImageToSwapchainStage::SetTargetImage(ManagedImage* newTargetImage)
    {
        mSourceImage = newTargetImage;
    }

    void ImageToSwapchainStage::RecordFrame(FrameRenderInfo& renderInfo)
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

        uint32_t        swapChainImageIndex = renderInfo.GetSwapchainImageIndex();
        VkCommandBuffer commandBuffer       = renderInfo.GetCommandBuffer();

        // Barrier: Grab swapchain image, change it into transfer dst layout
        barrier.srcAccessMask       = 0;                                               // since no memory operations happen before layout transition, we don't care of a srcMask
        barrier.dstAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;  // we block memory transfer to this swapchain image before layout transition
        barrier.oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = mContext->QueueGraphics;
        barrier.dstQueueFamilyIndex = mContext->QueueGraphics;
        barrier.image               = mContext->ContextSwapchain.SwapchainImages[swapChainImageIndex].Image;

        // srcStage: since we aquire a swapchain image, we don't need to wait for any commands to be processed, so we use top of pipe bit = wait for nothing
        // dstStage: we execute a transfer command later, so we block transfer until the layout has been transfered.
        vkCmdPipelineBarrier(commandBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

        ManagedImage::LayoutTransitionInfo layoutTransitionInfo;
        layoutTransitionInfo.CommandBuffer        = commandBuffer;
        layoutTransitionInfo.BarrierSrcAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // flush color attachment writes
        layoutTransitionInfo.BarrierDstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;          // block transfer writes before layout transition is over
        layoutTransitionInfo.OldImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
        layoutTransitionInfo.NewImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        layoutTransitionInfo.SrcQueueFamilyIndex  = mContext->QueueGraphics;
        layoutTransitionInfo.DstQueueFamilyIndex  = mContext->QueueGraphics;
        layoutTransitionInfo.SubresourceRange     = range;
        layoutTransitionInfo.SrcStage             = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // our source image was a color attachment in a previous pass
        layoutTransitionInfo.DstStage             = VK_PIPELINE_STAGE_TRANSFER_BIT;                 // we block transfer before layout transition is done
        mSourceImage->TransitionLayout(layoutTransitionInfo);


        VkImageSubresourceLayers layers = {};
        layers.aspectMask               = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        layers.mipLevel                 = 0;
        layers.baseArrayLayer           = 0;
        layers.layerCount               = 1;

        VkImageBlit blitRegion    = {};
        blitRegion.srcSubresource = layers;
        blitRegion.srcOffsets[0]  = VkOffset3D{.x = (int32_t)mContext->Swapchain.extent.width, .y = (int32_t)mContext->Swapchain.extent.height, .z = 0};
        blitRegion.srcOffsets[1]  = {.z = 1};
        blitRegion.dstSubresource = layers;
        blitRegion.dstOffsets[0]  = VkOffset3D{.x = (int32_t)mContext->Swapchain.extent.width, .y = (int32_t)mContext->Swapchain.extent.height, .z = 0};
        blitRegion.dstOffsets[1]  = {.z = 1};

        vkCmdBlitImage(commandBuffer, mSourceImage->GetImage(), VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       mContext->ContextSwapchain.SwapchainImages[swapChainImageIndex].Image, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion,
                       VkFilter::VK_FILTER_NEAREST);


        // Barrier: Change swapchain image to present layout, transfer it back to present queue
        barrier.srcAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;  // we wait for the memory transfer to happen
        barrier.dstAccessMask       = 0;                                               // we don't intend to do anything anymore with the resource so no commands need to be blocked
        barrier.oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcQueueFamilyIndex = mContext->QueueGraphics;
        barrier.dstQueueFamilyIndex = mContext->PresentQueue;

        vkCmdPipelineBarrier(commandBuffer,
                             VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,     // wait for transfer
                             VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  // block nothing
                             0, 0, nullptr, 0, nullptr, 1, &barrier);

        if(!!mPostCopy.AccessFlags && !!mPostCopy.ImageLayout)
        {
            // Return old image back
            ManagedImage::LayoutTransitionInfo layoutTransitionInfo;
            layoutTransitionInfo.CommandBuffer        = commandBuffer;
            layoutTransitionInfo.BarrierSrcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;  // flush color attachment writes
            layoutTransitionInfo.BarrierDstAccessMask = mPostCopy.AccessFlags;                           // block transfer writes before layout transition is over
            layoutTransitionInfo.OldImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            layoutTransitionInfo.NewImageLayout       = mPostCopy.ImageLayout;
            layoutTransitionInfo.SrcQueueFamilyIndex  = mContext->QueueGraphics;
            layoutTransitionInfo.DstQueueFamilyIndex  = mPostCopy.QueueFamilyIndex;
            layoutTransitionInfo.SubresourceRange     = range;
            layoutTransitionInfo.SrcStage             = VK_PIPELINE_STAGE_TRANSFER_BIT;      // our source image was a color attachment in a previous pass
            layoutTransitionInfo.DstStage             = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;  // we block transfer before layout transition is done
            mSourceImage->TransitionLayout(layoutTransitionInfo);
        }
    }

}  // namespace hsk