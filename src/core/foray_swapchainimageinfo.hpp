#pragma once
#include "../foray_vulkan.hpp"

namespace foray::core {
    struct SwapchainImageInfo
    {
        std::string Name      = "";
        VkImage     Image     = nullptr;
        VkImageView ImageView = nullptr;

        inline operator VkImage() const { return Image; }
        inline operator VkImageView() const { return ImageView; }
    };
}  // namespace foray::core