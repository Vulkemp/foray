#include "hsk_imagetoswapchain.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../memory/hsk_commandbuffer.hpp"


namespace hsk {
    void ImageToSwapchainStage::Init(const VkContext* context, ManagedImage* sourceImage)
    {
        mContext     = context;
        mSourceImage = sourceImage;
    }

    void ImageToSwapchainStage::OnResized(const VkExtent2D& extent, ManagedImage* newSourceImage) { mSourceImage = newSourceImage; }

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

        // Barrier: Grab swapchain image from present queue, change it into transfer dst layout
        barrier.srcAccessMask       = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
        barrier.dstAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = mContext->QueueGraphics;
        barrier.dstQueueFamilyIndex = mContext->QueueGraphics;
        barrier.image               = mContext->ContextSwapchain.SwapchainImages[swapChainImageIndex].Image;

        vkCmdPipelineBarrier(commandBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

        ManagedImage::LayoutTransitionInfo layoutTransitionInfo;
        layoutTransitionInfo.CommandBuffer        = commandBuffer;
        layoutTransitionInfo.BarrierSrcAccessMask = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
        layoutTransitionInfo.BarrierDstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
        layoutTransitionInfo.NewImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        layoutTransitionInfo.OldImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
        layoutTransitionInfo.SrcQueueFamilyIndex  = mContext->QueueGraphics;
        layoutTransitionInfo.DstQueueFamilyIndex  = mContext->QueueGraphics;
        layoutTransitionInfo.SubresourceRange     = range;
        layoutTransitionInfo.SrcStage             = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;  // TODO: these are wrong most likely
        layoutTransitionInfo.DstStage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
        mSourceImage->TransitionLayout(layoutTransitionInfo);


        // Copy one of the g-buffer images into the swapchain / TODO: This is not done
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

        vkCmdBlitImage(commandBuffer, mSourceImage->GetImage(), mSourceImage->GetImageLayout(),
                       mContext->ContextSwapchain.SwapchainImages[renderInfo.GetSwapchainImageIndex()].Image, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion,
                       VkFilter::VK_FILTER_NEAREST);


        // Barrier: Change swapchain image to present layout, transfer it back to present queue
        barrier.srcAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask       = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
        barrier.oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcQueueFamilyIndex = mContext->QueueGraphics;
        barrier.dstQueueFamilyIndex = mContext->PresentQueue;

        vkCmdPipelineBarrier(commandBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
                             0, nullptr, 1, &barrier);
    }

}  // namespace hsk