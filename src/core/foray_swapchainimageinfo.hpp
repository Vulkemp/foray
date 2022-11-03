#pragma once
#include "../foray_vulkan.hpp"

namespace foray::core {
    /// @brief Collects information for a swapchain image
    struct SwapchainImageInfo
    {
        /// @brief Debug name given
        std::string Name      = "";
        /// @brief VkImage
        VkImage     Image     = nullptr;
        /// @brief VkImageView
        VkImageView ImageView = nullptr;

        inline operator VkImage() const { return Image; }
        inline operator VkImageView() const { return ImageView; }
    };
}  // namespace foray::core