#pragma once
#include "../osi/foray_window.hpp"
#include <vkbootstrap/VkBootstrap.h>
#include "../foray_vulkan.hpp"
#include "../foray_vma.hpp"

namespace foray::core {

    /// @brief Used for easy queue access.
    struct Queue
    {
        VkQueue  Queue;
        uint32_t QueueFamilyIndex;

        operator VkQueue() const { return Queue; }
        operator uint32_t() const { return QueueFamilyIndex; }
    };

    struct SwapchainImage
    {
        VkImage       Image;
        VkImageView   ImageView;
        VkImageLayout ImageLayout;
    };

    struct SwapchainContext
    {
        foray::Window                 Window  = {};
        VkSurfaceKHR                Surface = {};
        std::vector<SwapchainImage> SwapchainImages{};
    };

    /// @brief Contains vulkan variables you want to pass around the application.
    struct VkContext
    {
        vkb::Instance       Instance{};
        vkb::PhysicalDevice PhysicalDevice{};
        vkb::Device         Device{};
        VmaAllocator        Allocator{};
        VkCommandPool       CommandPool{};
        Queue               QueueGraphics{};
        Queue               PresentQueue{};
        vkb::Swapchain      Swapchain{};
        SwapchainContext    ContextSwapchain{};
        VkCommandPool       TransferCommandPool{};
        Queue               TransferQueue{};
        bool                DebugEnabled;  // global flag indicating debug is enabled
        vkb::DispatchTable  DispatchTable;
    };
}  // namespace foray