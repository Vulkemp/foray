#pragma once
#include "../osi/hsk_window.hpp"
#include <vkbootstrap/VkBootstrap.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

/// @brief Contains vulkan variables you want to pass around the application.
struct VkContext
{
    vkb::Instance       Instance{};
    vkb::PhysicalDevice PhysicalDevice{};
    vkb::Device         Device{};
    VmaAllocator        Allocator{};
    VkCommandPool       CommandPool{};
    VkQueue             QueueGraphics{};
    vkb::Swapchain      Swapchain{};
    VkCommandPool       TransferCommandPool = nullptr;
    VkQueue             TransferQueue       = nullptr;
    bool                DebugEnabled;
    vkb::DispatchTable  DispatchTable;
    hsk::Window*        Window;
};