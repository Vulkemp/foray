#pragma once
#include "hsk_basics.hpp"
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {
    VkCommandBuffer createCommandBuffer(VkDevice device, VkCommandPool cmdpool, VkCommandBufferLevel level, bool begin = false);

    void beginCommandBuffer(VkCommandBuffer commandBuffer);

    void flushCommandBuffer(VkDevice device, VkCommandPool cmdpool, VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);

    void createBuffer(VmaAllocator            allocator,
                          VkBufferUsageFlags      usageFlags,
                          VmaAllocationCreateInfo allocInfo,
                          VmaAllocation*          allocation,
                          VkDeviceSize            size,
                          VkBuffer*               buffer,
                          void*                   data = nullptr);
}  // namespace hsk
