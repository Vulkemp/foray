#pragma once
#include <vkbootstrap/VkBootstrap.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

/// @brief Contains vulkan variables you want to pass around the application.
struct VkContext
{
    VkPhysicalDevice PhysicalDevice;
    VkDevice         Device{};
    VmaAllocator     Allocator{};
    VkCommandPool    CommandPool{};
    VkQueue          QueueGraphics{};
    vkb::Swapchain   Swapchain{};
    VkCommandPool    TransferCommandPool = nullptr;
    VkQueue          TransferQueue       = nullptr;
};