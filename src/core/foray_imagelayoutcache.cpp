#include "foray_imagelayoutcache.hpp"
#include "foray_managedimage.hpp"
// #include "../foray_logger.hpp"
// #include <nameof/nameof.hpp>

namespace foray::core {
    VkImageLayout ImageLayoutCache::Get(std::string_view name) const
    {
        std::string namecopy(name);
        const auto  iter = mLayoutCache.find(namecopy);
        if(iter != mLayoutCache.cend())
        {
            return iter->second;
        }
        return VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    }

    VkImageLayout ImageLayoutCache::Get(const ManagedImage& image) const
    {
        return Get(image.GetName());
    }

    VkImageLayout ImageLayoutCache::Get(const ManagedImage* image) const
    {
        return Get(image->GetName());
    }

    void ImageLayoutCache::Set(std::string_view name, VkImageLayout layout)
    {
        std::string namecopy(name);
        // if(layout < NAMEOF_ENUM_RANGE_MAX)
        // {
        //     logger()->debug("Layout [{}]: {}", name, NAMEOF_ENUM(layout));
        // }
        // else{
        //     logger()->debug("Layout [{}]: {}", name, "Unknown");
        // }
        mLayoutCache[namecopy] = layout;
    }
    void ImageLayoutCache::Set(const ManagedImage& image, VkImageLayout layout)
    {
        Set(image.GetName(), layout);
    }

    void ImageLayoutCache::Set(const ManagedImage* image, VkImageLayout layout)
    {
        Set(image->GetName(), layout);
    }

    void ImageLayoutCache::Set(std::string_view imageName, VkImageMemoryBarrier2& barrier, VkImageLayout newLayout)
    {
        barrier.oldLayout = Get(imageName);
        barrier.newLayout = newLayout;
        Set(imageName, newLayout);
    }
    void ImageLayoutCache::Set(const ManagedImage& image, VkImageMemoryBarrier2& barrier, VkImageLayout newLayout)
    {
        barrier.image = image.GetImage(), Set(image.GetName(), barrier, newLayout);
    }
    void ImageLayoutCache::Set(const ManagedImage* image, VkImageMemoryBarrier2& barrier, VkImageLayout newLayout)
    {
        barrier.image = image->GetImage(), Set(image->GetName(), barrier, newLayout);
    }

    void ImageLayoutCache::Set(std::string_view imageName, VkImageMemoryBarrier& barrier, VkImageLayout newLayout)
    {
        barrier.oldLayout = Get(imageName);
        barrier.newLayout = newLayout;
        Set(imageName, newLayout);
    }
    void ImageLayoutCache::Set(const ManagedImage& image, VkImageMemoryBarrier& barrier, VkImageLayout newLayout)
    {
        barrier.image = image.GetImage(), Set(image.GetName(), barrier, newLayout);
    }
    void ImageLayoutCache::Set(const ManagedImage* image, VkImageMemoryBarrier& barrier, VkImageLayout newLayout)
    {
        barrier.image = image->GetImage(), Set(image->GetName(), barrier, newLayout);
    }

    VkImageMemoryBarrier ImageLayoutCache::Set(std::string_view name, VkImage image, const Barrier& barrier)
    {
        VkImageLayout oldLayout = Get(name);
        Set(name, barrier.NewLayout);
        return VkImageMemoryBarrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                    .srcAccessMask       = barrier.SrcAccessMask,
                                    .dstAccessMask       = barrier.DstAccessMask,
                                    .oldLayout           = oldLayout,
                                    .newLayout           = barrier.NewLayout,
                                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    .image               = image,
                                    .subresourceRange    = barrier.SubresourceRange};
    }

    VkImageMemoryBarrier ImageLayoutCache::Set(const ManagedImage* image, const Barrier& barrier)
    {
        return Set(image->GetName(), image->GetImage(), barrier);
    }

    VkImageMemoryBarrier ImageLayoutCache::Set(const ManagedImage& image, const Barrier& barrier)
    {
        return Set(image.GetName(), image.GetImage(), barrier);
    }

    VkImageMemoryBarrier2 ImageLayoutCache::Set(std::string_view name, VkImage image, const Barrier2& barrier)
    {
        VkImageLayout oldLayout = Get(name);
        Set(name, barrier.NewLayout);
        return VkImageMemoryBarrier2{.sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                     .srcStageMask        = barrier.SrcStageMask,
                                     .srcAccessMask       = barrier.SrcAccessMask,
                                     .dstStageMask        = barrier.DstStageMask,
                                     .dstAccessMask       = barrier.DstAccessMask,
                                     .oldLayout           = oldLayout,
                                     .newLayout           = barrier.NewLayout,
                                     .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                     .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                     .image               = image,
                                     .subresourceRange    = barrier.SubresourceRange};
    }

    VkImageMemoryBarrier2 ImageLayoutCache::Set(const ManagedImage* image, const Barrier2& barrier)
    {
        return Set(image->GetName(), image->GetImage(), barrier);
    }

    VkImageMemoryBarrier2 ImageLayoutCache::Set(const ManagedImage& image, const Barrier2& barrier)
    {
        return Set(image.GetName(), image.GetImage(), barrier);
    }


}  // namespace foray::core
