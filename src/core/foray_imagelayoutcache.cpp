#include "foray_imagelayoutcache.hpp"
#include "foray_managedimage.hpp"

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
        barrier.image = image.GetImage(),
        Set(image.GetName(), barrier, newLayout);
    }
    void ImageLayoutCache::Set(const ManagedImage* image, VkImageMemoryBarrier2& barrier, VkImageLayout newLayout)
    {
        barrier.image = image->GetImage(),
        Set(image->GetName(), barrier, newLayout);
    }

}  // namespace foray::core
