#pragma once
#include "../foray_vulkan.hpp"
#include "foray_core_declares.hpp"
#include <unordered_map>

namespace foray::core {
    class ImageLayoutCache
    {
      public:
        VkImageLayout Get(std::string_view imageName) const;
        VkImageLayout Get(const ManagedImage& image) const;
        VkImageLayout Get(const ManagedImage* image) const;

        void Set(std::string_view imageName, VkImageLayout layout);
        void Set(const ManagedImage& image, VkImageLayout layout);
        void Set(const ManagedImage* image, VkImageLayout layout);

        void Set(std::string_view imageName, VkImageMemoryBarrier2& barrier, VkImageLayout newLayout);
        void Set(const ManagedImage& image, VkImageMemoryBarrier2& barrier, VkImageLayout newLayout);
        void Set(const ManagedImage* image, VkImageMemoryBarrier2& barrier, VkImageLayout newLayout);
        void Set(std::string_view imageName, VkImageMemoryBarrier& barrier, VkImageLayout newLayout);
        void Set(const ManagedImage& image, VkImageMemoryBarrier& barrier, VkImageLayout newLayout);
        void Set(const ManagedImage* image, VkImageMemoryBarrier& barrier, VkImageLayout newLayout);

        struct Barrier
        {
            VkAccessFlags           SrcAccessMask    = VkAccessFlagBits::VK_ACCESS_NONE;
            VkAccessFlags           DstAccessMask    = VkAccessFlagBits::VK_ACCESS_NONE;
            VkImageSubresourceRange SubresourceRange = VkImageSubresourceRange{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1U, .layerCount = 1U};
            VkImageLayout           NewLayout;
        };

        struct Barrier2
        {
            VkPipelineStageFlags2   SrcStageMask     = VK_PIPELINE_STAGE_2_NONE;
            VkAccessFlags2          SrcAccessMask    = VK_ACCESS_2_NONE;
            VkPipelineStageFlags2   DstStageMask     = VK_PIPELINE_STAGE_2_NONE;
            VkAccessFlags2          DstAccessMask    = VK_ACCESS_2_NONE;
            VkImageSubresourceRange SubresourceRange = VkImageSubresourceRange{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1U, .layerCount = 1U};
            VkImageLayout           NewLayout;
        };

        VkImageMemoryBarrier  Set(std::string_view name, VkImage image, const Barrier& barrier);
        VkImageMemoryBarrier  Set(const ManagedImage* image, const Barrier& barrier);
        VkImageMemoryBarrier  Set(const ManagedImage& image, const Barrier& barrier);
        VkImageMemoryBarrier2 Set(std::string_view name, VkImage image, const Barrier2& barrier);
        VkImageMemoryBarrier2 Set(const ManagedImage* image, const Barrier2& barrier);
        VkImageMemoryBarrier2 Set(const ManagedImage& image, const Barrier2& barrier);

      protected:
        std::unordered_map<std::string, VkImageLayout> mLayoutCache;
    };
}  // namespace foray::core
