#include "hsk_vkHelpers.hpp"
#include <nameof/nameof.hpp>

namespace hsk {
    std::string_view PrintVkResult(VkResult result) { return NAMEOF_ENUM(result); }

    VkCommandBuffer CreateCommandBuffer(VkDevice device, VkCommandPool cmdpool, VkCommandBufferLevel level, bool begin)
    {
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool        = cmdpool;
        cmdBufAllocateInfo.level              = level;
        cmdBufAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer cmdBuffer;
        AssertVkResult(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));

        // If requested, also start recording for the new command buffer
        if(begin)
        {
            VkCommandBufferBeginInfo commandBufferBI{};
            commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            AssertVkResult(vkBeginCommandBuffer(cmdBuffer, &commandBufferBI));
        }

        return cmdBuffer;
    }

    void BeginCommandBuffer(VkCommandBuffer commandBuffer)
    {
        VkCommandBufferBeginInfo commandBufferBI{};
        commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        AssertVkResult(vkBeginCommandBuffer(commandBuffer, &commandBufferBI));
    }

    void FlushCommandBuffer(VkDevice device, VkCommandPool cmdpool, VkCommandBuffer commandBuffer, VkQueue queue, bool free)
    {
        AssertVkResult(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffer;

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence;
        AssertVkResult(vkCreateFence(device, &fenceInfo, nullptr, &fence));

        // Submit to the queue
        AssertVkResult(vkQueueSubmit(queue, 1, &submitInfo, fence));
        // Wait for the fence to signal that command buffer has finished executing
        AssertVkResult(vkWaitForFences(device, 1, &fence, VK_TRUE, 100000000000));

        vkDestroyFence(device, fence, nullptr);

        if(free)
        {
            vkFreeCommandBuffers(device, cmdpool, 1, &commandBuffer);
        }
    }

    void SetVulkanObjectName(const VkContext* context, VkObjectType objectType, const void* objectHandle, const std::string_view& name)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{.sType        = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                                               .pNext        = nullptr,
                                               .objectType   = objectType,
                                               .objectHandle = reinterpret_cast<uint64_t>(objectHandle),
                                               .pObjectName  = name.data()};
        context->DispatchTable.setDebugUtilsObjectNameEXT(&nameInfo);
    }


}  // namespace hsk