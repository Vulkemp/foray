#include "hsk_commandbuffer.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {

    VkCommandBuffer SingleTimeCommandBuffer::Create(const VkContext* context, VkCommandBufferLevel cmdBufferLvl, bool begin = false)
    {
        mContext = context;
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool        = context->CommandPool;
        cmdBufAllocateInfo.level              = cmdBufferLvl;
        cmdBufAllocateInfo.commandBufferCount = 1;

        AssertVkResult(vkAllocateCommandBuffers(context->Device, &cmdBufAllocateInfo, &mCommandBuffer));

        // If requested, also start recording for the new command buffer
        if(begin)
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            AssertVkResult(vkBeginCommandBuffer(mCommandBuffer, &beginInfo));
        }

        return mCommandBuffer;
    }

    void SingleTimeCommandBuffer::Begin()
    {
        VkCommandBufferBeginInfo commandBufferBI{};
        commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        AssertVkResult(vkBeginCommandBuffer(mCommandBuffer, &commandBufferBI));
    }

    void SingleTimeCommandBuffer::Flush(bool free)
    {
        AssertVkResult(vkEndCommandBuffer(mCommandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &mCommandBuffer;

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence;
        AssertVkResult(vkCreateFence(mContext->Device, &fenceInfo, nullptr, &fence));

        // Submit to the queue
        AssertVkResult(vkQueueSubmit(mContext->QueueGraphics, 1, &submitInfo, fence)); // TODO: Use graphics queue for all command buffer submits??? seems wrong.
        // Wait for the fence to signal that command buffer has finished executing
        AssertVkResult(vkWaitForFences(mContext->Device, 1, &fence, VK_TRUE, 100000000000));

        vkDestroyFence(mContext->Device, fence, nullptr);

        if(free)
        {
            vkFreeCommandBuffers(mContext->Device, mContext->CommandPool, 1, &mCommandBuffer);
        }
    }

}  // namespace hsk