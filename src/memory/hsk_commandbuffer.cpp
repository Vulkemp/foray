#include "hsk_commandbuffer.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {

    VkCommandBuffer CommandBuffer::Create(const VkContext* context, VkCommandBufferLevel cmdBufferLvl, bool begin)
    {
        mContext = context;
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool        = context->CommandPool;
        cmdBufAllocateInfo.level              = cmdBufferLvl;
        cmdBufAllocateInfo.commandBufferCount = 1;

        AssertVkResult(vkAllocateCommandBuffers(context->Device, &cmdBufAllocateInfo, &mCommandBuffer));

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence;
        AssertVkResult(vkCreateFence(mContext->Device, &fenceInfo, nullptr, &mFence));

        // If requested, also start recording for the new command buffer
        if(begin)
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            AssertVkResult(vkBeginCommandBuffer(mCommandBuffer, &beginInfo));
        }

        return mCommandBuffer;
    }

    void CommandBuffer::Begin()
    {
        VkCommandBufferBeginInfo commandBufferBI{};
        commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        AssertVkResult(vkBeginCommandBuffer(mCommandBuffer, &commandBufferBI));
    }

    void CommandBuffer::Submit(bool fireAndForget)
    {
        AssertVkResult(vkEndCommandBuffer(mCommandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &mCommandBuffer;

        // Submit to the queue
        AssertVkResult(vkQueueSubmit(mContext->TransferQueue, 1, &submitInfo, mFence));  // TODO: Use graphics queue for all command buffer submits??? seems wrong.
        if(!fireAndForget)
        {
            // Wait for the fence to signal that command buffer has finished executing
            AssertVkResult(vkWaitForFences(mContext->Device, 1, &mFence, VK_TRUE, 100000000000));
            AssertVkResult(vkResetFences(mContext->Device, 1, &mFence));
        }
    }

    void CommandBuffer::FlushAndReset()
    {
        Submit();
        AssertVkResult(vkResetCommandBuffer(mCommandBuffer, 0));
    }

    void CommandBuffer::WaitForCompletion()
    {
        if(!mFence || !mCommandBuffer)
        {
            return;
        }
        // Wait for the fence to signal that command buffer has finished executing
        AssertVkResult(vkWaitForFences(mContext->Device, 1, &mFence, VK_TRUE, 100000000000));
        AssertVkResult(vkResetFences(mContext->Device, 1, &mFence));
    }
    void CommandBuffer::Cleanup()
    {
        if(mCommandBuffer)
        {
            vkFreeCommandBuffers(mContext->Device, mContext->CommandPool, 1, &mCommandBuffer);
            mCommandBuffer = nullptr;
        }
        if(mFence)
        {
            vkDestroyFence(mContext->Device, mFence, nullptr);
            mFence = nullptr;
        }
    }

}  // namespace hsk