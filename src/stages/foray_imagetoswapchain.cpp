#include "foray_imagetoswapchain.hpp"

namespace foray::stages {
    void ImageToSwapchainStage::Init(const core::VkContext* context, core::ManagedImage* sourceImage)
    {
        mContext     = context;
        mSourceImage = sourceImage;
    }

    void ImageToSwapchainStage::SetTargetImage(core::ManagedImage* newTargetImage)
    {
        mSourceImage = newTargetImage;
    }

    void ImageToSwapchainStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        uint32_t swapChainImageIndex = renderInfo.GetInFlightFrame()->GetSwapchainImageIndex();

        const core::SwapchainImage& swapImage = mContext->ContextSwapchain.SwapchainImages[swapChainImageIndex];

        core::ImageLayoutCache::Barrier2 swapImgMemBarrier{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                           .SrcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                           .DstStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                                           .DstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                                           .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};

        core::ImageLayoutCache::Barrier2 srcImgMemBarrier{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                          .SrcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                          .DstStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                                          .DstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                                                          .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL};

        std::vector<VkImageMemoryBarrier2> barriers;
        barriers.reserve(2);
        barriers.push_back(renderInfo.GetImageLayoutCache().Set(swapImage.Name, swapImage.Image, swapImgMemBarrier));
        barriers.push_back(renderInfo.GetImageLayoutCache().Set(mSourceImage, srcImgMemBarrier));

        VkDependencyInfo depInfo{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = (uint32_t)barriers.size(), .pImageMemoryBarriers = barriers.data()};

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