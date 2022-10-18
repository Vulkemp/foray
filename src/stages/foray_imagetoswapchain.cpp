#include "foray_imagetoswapchain.hpp"

namespace foray::stages {
    void ImageToSwapchainStage::Init(const core::VkContext* context, core::ManagedImage* sourceImage, const PostCopy& postcopy)
    {
        mContext     = context;
        mSourceImage = sourceImage;
        mPostCopy    = postcopy;
    }

    void ImageToSwapchainStage::Init(const core::VkContext* context, core::ManagedImage* sourceImage)
    {
        mContext     = context;
        mSourceImage = sourceImage;
    }

    void ImageToSwapchainStage::OnResized(const VkExtent2D& extent, core::ManagedImage* newSourceImage)
    {
        mSourceImage = newSourceImage;
    }

    void ImageToSwapchainStage::SetTargetImage(core::ManagedImage* newTargetImage)
    {
        mSourceImage = newTargetImage;
    }

    void ImageToSwapchainStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        uint32_t swapChainImageIndex = renderInfo.GetInFlightFrame()->GetSwapchainImageIndex();

        VkImage swapImage = mContext->ContextSwapchain.SwapchainImages[swapChainImageIndex].Image;

        VkImageMemoryBarrier2 swapImgMemBarrier{
            .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask        = VK_PIPELINE_STAGE_2_BLIT_BIT,
            .dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = swapImage,
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                },
        };

        VkImageMemoryBarrier2 srcImgMemBarrier{
            .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask       = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask        = VK_PIPELINE_STAGE_2_BLIT_BIT,
            .dstAccessMask       = VK_ACCESS_2_TRANSFER_READ_BIT,
            .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = mSourceImage->GetImage(),
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                },
        };

        std::vector<VkImageMemoryBarrier2> barriers({swapImgMemBarrier, srcImgMemBarrier});

        VkDependencyInfo depInfo{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                 .imageMemoryBarrierCount = (uint32_t)barriers.size(),
                                 .pImageMemoryBarriers    = barriers.data()};

        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

        VkImageSubresourceLayers layers{
            .aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = 0,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        };

        VkExtent2D size = mContext->Swapchain.extent;

        VkImageBlit blitRegion{
            .srcSubresource = layers,
            .srcOffsets     = {VkOffset3D{}, VkOffset3D{(int32_t)size.width, (int32_t)size.height, 1}},
            .dstSubresource = layers,
            .dstOffsets     = {VkOffset3D{}, VkOffset3D{(int32_t)size.width, (int32_t)size.height, 1}},
        };

        vkCmdBlitImage(cmdBuffer, mSourceImage->GetImage(), VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapImage, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                       &blitRegion, VkFilter::VK_FILTER_NEAREST);
    }

}  // namespace foray::stages