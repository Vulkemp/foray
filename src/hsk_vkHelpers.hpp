#pragma once
#include "hsk_basics.hpp"
#include <string_view>
#include <vulkan/vulkan.h>

namespace hsk {

    std::string_view PrintVkResult(VkResult result);

    inline void AssertVkResult(VkResult result, const std::source_location location = std::source_location::current())
    {
        if(result != VK_SUCCESS)
        {
            Exception::Throw(location, "VkResult Assertion Failed: VkResult::{}", PrintVkResult(result));
        }
    }

    VkCommandBuffer CreateCommandBuffer(VkDevice device, VkCommandPool cmdpool, VkCommandBufferLevel level, bool begin = false);

    void BeginCommandBuffer(VkCommandBuffer commandBuffer);

    void FlushCommandBuffer(VkDevice device, VkCommandPool cmdpool, VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);
}  // namespace hsk