#pragma once
#include "../hsk_rtrpf.hpp"
#include <vulkan/vulkan.h>

/// @brief Contains vulkan variables you want to pass around the application.
struct VkContext
{
    VkDevice  Device{};
    VmaAllocator  Allocator{};
    VkCommandPool CommandPool{};
};