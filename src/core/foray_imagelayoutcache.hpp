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

      protected:
        std::unordered_map<std::string, VkImageLayout> mLayoutCache;
    };
}  // namespace foray::core
