#include "imagelayoutcache.hpp"
#include "managedimage.hpp"
// #include "../logger.hpp"
// #include <nameof/nameof.hpp>

namespace foray::core {
    VkImageLayout ImageLayoutCache::Get(VkImage image) const
    {
        const auto  iter = mLayoutCache.find(image);
        if(iter != mLayoutCache.cend())
        {
            return iter->second;
        }
        return VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    }

    void ImageLayoutCache::Set(VkImage image, VkImageLayout layout)
    {
        Assert(!!image, "Cannot track imagelayout for nullptr!");
        mLayoutCache[image] = layout;
    }

    VkImageLayout ImageLayoutCache::Get(const ManagedImage& image) const
    {
        return Get(image.GetImage());
    }

    VkImageLayout ImageLayoutCache::Get(const ManagedImage* image) const
    {
        return Get(image->GetImage());
    }

    void ImageLayoutCache::Set(const ManagedImage& image, VkImageLayout layout)
    {
        Set(image.GetImage(), layout);
    }

    void ImageLayoutCache::Set(const ManagedImage* image, VkImageLayout layout)
    {
        Set(image->GetImage(), layout);
    }

    VkImageMemoryBarrier ImageLayoutCache::MakeBarrier(VkImage image, const Barrier& barrier)
    {
        VkImageLayout oldLayout = Get(image);
        Set(image, barrier.NewLayout);
        return VkImageMemoryBarrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                    .srcAccessMask       = barrier.SrcAccessMask,
                                    .dstAccessMask       = barrier.DstAccessMask,
                                    .oldLayout           = oldLayout,
                                    .newLayout           = barrier.NewLayout,
                                    .srcQueueFamilyIndex = barrier.SrcQueueFamilyIndex,
                                    .dstQueueFamilyIndex = barrier.DstQueueFamilyIndex,
                                    .image               = image,
                                    .subresourceRange    = barrier.SubresourceRange};
    }

    VkImageMemoryBarrier ImageLayoutCache::MakeBarrier(const ManagedImage* image, const Barrier& barrier)
    {
        return MakeBarrier(image->GetImage(), barrier);
    }

    VkImageMemoryBarrier ImageLayoutCache::MakeBarrier(const ManagedImage& image, const Barrier& barrier)
    {
        return MakeBarrier(image.GetImage(), barrier);
    }

    VkImageMemoryBarrier2 ImageLayoutCache::MakeBarrier(VkImage image, const Barrier2& barrier)
    {
        VkImageLayout oldLayout = Get(image);
        Set(image, barrier.NewLayout);
        return VkImageMemoryBarrier2{.sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                     .srcStageMask        = barrier.SrcStageMask,
                                     .srcAccessMask       = barrier.SrcAccessMask,
                                     .dstStageMask        = barrier.DstStageMask,
                                     .dstAccessMask       = barrier.DstAccessMask,
                                     .oldLayout           = oldLayout,
                                     .newLayout           = barrier.NewLayout,
                                     .srcQueueFamilyIndex = barrier.SrcQueueFamilyIndex,
                                     .dstQueueFamilyIndex = barrier.DstQueueFamilyIndex,
                                     .image               = image,
                                     .subresourceRange    = barrier.SubresourceRange};
    }

    VkImageMemoryBarrier2 ImageLayoutCache::MakeBarrier(const ManagedImage* image, const Barrier2& barrier)
    {
        return MakeBarrier(image->GetImage(), barrier);
    }

    VkImageMemoryBarrier2 ImageLayoutCache::MakeBarrier(const ManagedImage& image, const Barrier2& barrier)
    {
        return MakeBarrier(image.GetImage(), barrier);
    }

    void ImageLayoutCache::CmdBarrier(VkCommandBuffer      cmdBuffer,
                                      VkImage              image,
                                      const Barrier&       barrier,
                                      VkPipelineStageFlags srcStageMask,
                                      VkPipelineStageFlags dstStageMask,
                                      VkDependencyFlags    depFlags)
    {
        VkImageMemoryBarrier vkBarrier = MakeBarrier(image, barrier);
        vkCmdPipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, depFlags, 0, nullptr, 0, nullptr, 1U, &vkBarrier);
    }
    void ImageLayoutCache::CmdBarrier(VkCommandBuffer      cmdBuffer,
                                      const ManagedImage*  image,
                                      const Barrier&       barrier,
                                      VkPipelineStageFlags srcStageMask,
                                      VkPipelineStageFlags dstStageMask,
                                      VkDependencyFlags    depFlags)
    {
        VkImageMemoryBarrier vkBarrier = MakeBarrier(image, barrier);
        vkCmdPipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, depFlags, 0, nullptr, 0, nullptr, 1U, &vkBarrier);
    }
    void ImageLayoutCache::CmdBarrier(VkCommandBuffer      cmdBuffer,
                                      const ManagedImage&  image,
                                      const Barrier&       barrier,
                                      VkPipelineStageFlags srcStageMask,
                                      VkPipelineStageFlags dstStageMask,
                                      VkDependencyFlags    depFlags)
    {
        VkImageMemoryBarrier vkBarrier = MakeBarrier(image, barrier);
        vkCmdPipelineBarrier(cmdBuffer, srcStageMask, dstStageMask, depFlags, 0, nullptr, 0, nullptr, 1U, &vkBarrier);
    }
    void ImageLayoutCache::CmdBarrier(VkCommandBuffer cmdBuffer, VkImage image, const Barrier2& barrier, VkDependencyFlags depFlags)
    {
        VkImageMemoryBarrier2 vkBarrier = MakeBarrier(image, barrier);
        VkDependencyInfo      depInfo
        {
            .sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags = depFlags,
            .imageMemoryBarrierCount = 1U,
            .pImageMemoryBarriers = &vkBarrier
        };
        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
    }
    void ImageLayoutCache::CmdBarrier(VkCommandBuffer cmdBuffer, const ManagedImage* image, const Barrier2& barrier, VkDependencyFlags depFlags)
    {
        CmdBarrier(cmdBuffer, image->GetImage(), barrier, depFlags);
    }
    void ImageLayoutCache::CmdBarrier(VkCommandBuffer cmdBuffer, const ManagedImage& image, const Barrier2& barrier, VkDependencyFlags depFlags)
    {
        CmdBarrier(cmdBuffer, image.GetImage(), barrier, depFlags);
    }

}  // namespace foray::core
