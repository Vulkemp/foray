#include "historyimage.hpp"
#include "../base/framerenderinfo.hpp"
#include <spdlog/fmt/fmt.h>

namespace foray::util {
    HistoryImage::HistoryImage(core::Context* context, core::ManagedImage* source, VkImageUsageFlags additionalUsageFlags)
    {
        mSource                           = source;
        core::ManagedImage::CreateInfo ci = source->GetCreateInfo();
        ci.Name                           = fmt::format("History.{}", ci.Name);
        ci.ImageCI.usage |= VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT | additionalUsageFlags;
        mHistory.New(context, ci);
        mHistoricLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    }

    void HistoryImage::Resize(const VkExtent2D& size)
    {
        mHistory.Resize(size);
        mHistoricLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    }

    void HistoryImage::ApplyToLayoutCache(core::ImageLayoutCache& layoutCache)
    {
        layoutCache.Set(mHistory.Get(), mHistoricLayout);
    }
    void HistoryImage::CmdCopySourceToHistory(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        {  // Pipeline Barrier
            std::array<VkImageMemoryBarrier2, 2U> vkBarriers({
                renderInfo.GetImageLayoutCache().MakeBarrier(mSource,
                                                             core::ImageLayoutCache::Barrier2{
                                                                 .SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                                 .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                                 .DstStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT,
                                                                 .DstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                                                                 .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                             }),
                renderInfo.GetImageLayoutCache().MakeBarrier(mHistory.Get(),
                                                             core::ImageLayoutCache::Barrier2{
                                                                 .SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                                 .SrcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
                                                                 .DstStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT,
                                                                 .DstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                                                 .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                             }),
            });

            mHistoricLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            VkDependencyInfo depInfo{
                .sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = (uint32_t)vkBarriers.size(), .pImageMemoryBarriers = vkBarriers.data()};

            vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
        }

        {  // CmdCopyImage
            VkImageSubresourceLayers layer{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1};

            VkImageCopy2 imageCopy{
                .sType          = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_COPY_2,
                .srcSubresource = layer,
                .srcOffset      = VkOffset3D{},
                .dstSubresource = layer,
                .dstOffset      = VkOffset3D{},
                .extent         = mSource->GetExtent3D(),
            };

            VkCopyImageInfo2 copyInfo{
                .sType          = VkStructureType::VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
                .srcImage       = mSource->GetImage(),
                .srcImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .dstImage       = mHistory->GetImage(),
                .dstImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .regionCount    = 1U,
                .pRegions       = &imageCopy,
            };

            vkCmdCopyImage2(cmdBuffer, &copyInfo);
        }
    }

    void HistoryImage::sMultiCopySourceToHistory(const std::vector<HistoryImage*>& historyImages, VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        {  // Barriers
            std::vector<VkImageMemoryBarrier2> vkBarriers;
            vkBarriers.reserve(historyImages.size() * 2);

            for(HistoryImage* image : historyImages)
            {
                vkBarriers.push_back(renderInfo.GetImageLayoutCache().MakeBarrier(image->mSource, core::ImageLayoutCache::Barrier2{
                                                                                                      .SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                                                                      .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                                                                      .DstStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT,
                                                                                                      .DstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                                                                                                      .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                                                                  }));
                vkBarriers.push_back(renderInfo.GetImageLayoutCache().MakeBarrier(image->mHistory.Get(), core::ImageLayoutCache::Barrier2{
                                                                                                       .SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                                                                       .SrcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
                                                                                                       .DstStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT,
                                                                                                       .DstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                                                                                       .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                                                   }));

                image->SetHistoricLayout(VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            }

            VkDependencyInfo depInfo{
                .sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = (uint32_t)vkBarriers.size(), .pImageMemoryBarriers = vkBarriers.data()};

            vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
        }

        /// CmdCopyImage
        for(HistoryImage* image : historyImages)
        {

            VkImageSubresourceLayers layer{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1};

            VkImageCopy2 imageCopy{
                .sType          = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_COPY_2,
                .srcSubresource = layer,
                .srcOffset      = VkOffset3D{},
                .dstSubresource = layer,
                .dstOffset      = VkOffset3D{},
                .extent         = image->mSource->GetExtent3D(),
            };

            VkCopyImageInfo2 copyInfo{
                .sType          = VkStructureType::VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
                .srcImage       = image->mSource->GetImage(),
                .srcImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .dstImage       = image->mHistory->GetImage(),
                .dstImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .regionCount    = 1U,
                .pRegions       = &imageCopy,
            };

            vkCmdCopyImage2(cmdBuffer, &copyInfo);
        }
    }

}  // namespace foray::util
