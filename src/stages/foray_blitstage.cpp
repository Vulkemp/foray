#include "foray_blitstage.hpp"
#include "../foray_logger.hpp"

namespace foray::stages {
    void BlitStage::Init(core::ManagedImage* srcImage, core::ManagedImage* dstImage)
    {
        SetSrcImage(srcImage);
        SetDstImage(dstImage);
    }
    void BlitStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        if((!mSrcImage || !mSrcImage->Exists()) && (!mSrcVkImage || (mSrcImageSize.width * mSrcImageSize.height) == 0))
        {
#ifdef FORAY_DEBUG
            logger()->warn("[BlitStage::RecordFrame] Blit Image Stage skipped: SrcImage not set or source area zero!");
#endif
            return;
        }
        if(((!mDstImage || !mDstImage->Exists()) && (!mDstVkImage || (mDstImageSize.width * mDstImageSize.height) == 0)))
        {
#ifdef FORAY_DEBUG
            logger()->warn("[BlitStage::RecordFrame] Blit Image Stage skipped: DstImage not set or dest area zero!");
#endif
            return;
        }

        core::ImageLayoutCache::Barrier2 srcImgMemBarrier{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                          .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                          .DstStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                                          .DstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                                                          .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL};

        core::ImageLayoutCache::Barrier2 dstImgMemBarrier{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                          .SrcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                          .DstStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                                          .DstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                                          .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};

        std::vector<VkImageMemoryBarrier2> barriers;
        barriers.reserve(2);
        if(!!mSrcImage)
        {
            barriers.push_back(renderInfo.GetImageLayoutCache().MakeBarrier(mSrcImage, srcImgMemBarrier));
        }
        else
        {
            barriers.push_back(renderInfo.GetImageLayoutCache().MakeBarrier(mSrcVkImage, srcImgMemBarrier));
        }
        if(!!mDstImage)
        {
            barriers.push_back(renderInfo.GetImageLayoutCache().MakeBarrier(mDstImage, dstImgMemBarrier));
        }
        else
        {
            barriers.push_back(renderInfo.GetImageLayoutCache().MakeBarrier(mDstVkImage, dstImgMemBarrier));
        }

        VkDependencyInfo depInfo{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = (uint32_t)barriers.size(), .pImageMemoryBarriers = barriers.data()};

        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

        VkImageBlit2 imageBlit{};

        ConfigureBlitRegion(imageBlit);

        VkBlitImageInfo2 blitInfo{.sType          = VkStructureType::VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                                  .srcImage       = !!mSrcImage ? mSrcImage->GetImage() : mSrcVkImage,
                                  .srcImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  .dstImage       = !!mDstImage ? mDstImage->GetImage() : mDstVkImage,
                                  .dstImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  .regionCount    = 1U,
                                  .pRegions       = &imageBlit,
                                  .filter         = mFilter};

        vkCmdBlitImage2(cmdBuffer, &blitInfo);
    }
    void BlitStage::SetSrcImage(core::ManagedImage* image)
    {
        if(!!image && image->Exists())
        {
            mSrcImage = image;
        }
        else
        {
            SetSrcImage(nullptr, VkExtent2D{});
        }
    }
    void BlitStage::SetSrcImage(VkImage image, VkExtent2D size)
    {
        mSrcImage     = nullptr;
        mSrcVkImage   = image;
        mSrcImageSize = size;
    }
    void BlitStage::SetDstImage(core::ManagedImage* image)
    {
        if(!!image && image->Exists())
        {
            mDstImage = image;
        }
        else
        {
            SetDstImage(nullptr, VkExtent2D{});
        }
    }
    void BlitStage::SetDstImage(VkImage image, VkExtent2D size)
    {
        mDstImage     = nullptr;
        mDstVkImage   = image;
        mDstImageSize = size;
    }

    void BlitStage::ConfigureBlitRegion(VkImageBlit2& imageBlit)
    {
        int32_t srcWidth;
        int32_t srcHeight;
        int32_t dstWidth;
        int32_t dstHeight;
        if(!!mSrcImage)
        {
            srcWidth  = (int32_t)mSrcImage->GetExtent3D().width;
            srcHeight = (int32_t)mSrcImage->GetExtent3D().height;
        }
        else
        {
            srcWidth  = (int32_t)mSrcImageSize.width;
            srcHeight = (int32_t)mSrcImageSize.height;
        }
        if(!!mDstImage)
        {
            dstWidth  = (int32_t)mDstImage->GetExtent3D().width;
            dstHeight = (int32_t)mDstImage->GetExtent3D().height;
        }
        else
        {
            dstWidth  = (int32_t)mDstImageSize.width;
            dstHeight = (int32_t)mDstImageSize.height;
        }

        VkOffset3D dst0{.z = 0};
        VkOffset3D dst1{.z = 1};

        dst0.x = mFlipX ? dstWidth : 0;
        dst1.x = mFlipX ? 0 : dstWidth;
        dst0.y = mFlipY ? dstHeight : 0;
        dst1.y = mFlipY ? 0 : dstHeight;

        imageBlit = VkImageBlit2{.sType          = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                                 .srcSubresource = VkImageSubresourceLayers{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
                                 .srcOffsets     = {VkOffset3D{0, 0, 0}, VkOffset3D{srcWidth, srcHeight, 1}},
                                 .dstSubresource = VkImageSubresourceLayers{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
                                 .dstOffsets     = {dst0, dst1}};
    }
}  // namespace foray::stages
