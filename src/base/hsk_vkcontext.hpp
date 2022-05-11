#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

/// @brief Contains vulkan variables you want to pass around the application.
struct VkContext
{
    VkDevice      Device{};
    VmaAllocator  Allocator{};
    VkCommandPool CommandPool{};
    VkQueue       QueueGraphics{};
};