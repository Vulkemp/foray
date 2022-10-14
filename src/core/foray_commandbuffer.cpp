#include "foray_commandbuffer.hpp"
#include "../foray_vulkan.hpp"

namespace foray::core {

    VkCommandBuffer CommandBuffer::Create(const VkContext* context, VkCommandBufferLevel cmdBufferLvl, bool begin)
    {
        return Create(context->Device, context->CommandPool, cmdBufferLvl, begin);
    }

    VkCommandBuffer CommandBuffer::Create(VkDevice device, VkCommandPool cmdPool, VkCommandBufferLevel cmdBufferLvl, bool begin)
    {
        mDevice = device;
        mPool   = cmdPool;
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool        = mPool;
        cmdBufAllocateInfo.level              = cmdBufferLvl;
        cmdBufAllocateInfo.commandBufferCount = 1;

        AssertVkResult(vkAllocateCommandBuffers(mDevice, &cmdBufAllocateInfo, &mCommandBuffer));

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
        mIsRecording = true;
    }

    void CommandBuffer::End()
    {
        AssertVkResult(vkEndCommandBuffer(mCommandBuffer));
        mIsRecording = false;
    }

    void CommandBuffer::Reset(VkCommandBufferResetFlags flags)
    {
        AssertVkResult(vkResetCommandBuffer(mCommandBuffer, flags));
    }

    void CommandBuffer::Destroy()
    {
        if(!!mCommandBuffer)
        {
            vkFreeCommandBuffers(mDevice, mPool, 1, &mCommandBuffer);
            mCommandBuffer = nullptr;
        }
    }

    /// @brief Create based on the contexts device, command pool and queue.
    VkCommandBuffer HostCommandBuffer::Create(const VkContext* context, VkCommandBufferLevel cmdBufferLvl, bool begin)
    {
        return Create(context->Device, context->CommandPool, context->QueueGraphics, cmdBufferLvl, begin);
    }
    /// @brief Create based on custom vulkan context
    VkCommandBuffer HostCommandBuffer::Create(
        VkDevice device, VkCommandPool cmdPool, Queue queue, VkCommandBufferLevel cmdBufferLvl, bool begin)
    {
        mQueue = queue;
        CommandBuffer::Create(device, cmdPool, cmdBufferLvl, begin);

        VkFenceCreateInfo fenceCi{.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        vkCreateFence(mDevice, &fenceCi, nullptr, &mFence);

        return mCommandBuffer;
    }

    void HostCommandBuffer::Submit()
    {
        if(mIsRecording)
        {
            End();
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &mCommandBuffer;

        // Submit to the queue
        AssertVkResult(vkQueueSubmit(mQueue, 1, &submitInfo, mFence));
    }
    void HostCommandBuffer::SubmitAndWait()
    {
        Submit();
        WaitForCompletion();
    }
    bool HostCommandBuffer::HasCompleted()
    {
        VkResult result = vkGetFenceStatus(mDevice, mFence);
        if(result == VK_NOT_READY)
        {
            return false;
        }
        AssertVkResult(result);
        return true;
    }
    void HostCommandBuffer::WaitForCompletion()
    {
        // Wait for the fence to signal that command buffer has finished executing
        AssertVkResult(vkWaitForFences(mDevice, 1, &mFence, VK_TRUE, UINT64_MAX));
        AssertVkResult(vkResetFences(mDevice, 1, &mFence));
    }

    void HostCommandBuffer::Destroy()
    {
        if(!!mFence)
        {
            vkDestroyFence(mDevice, mFence, nullptr);
            mFence = nullptr;
        }
        CommandBuffer::Destroy();
    }

    SemaphoreSubmit SemaphoreSubmit::Binary(VkSemaphore semaphore, VkPipelineStageFlags2 waitStage)
    {
        return SemaphoreSubmit{.SemaphoreType = VkSemaphoreType::VK_SEMAPHORE_TYPE_BINARY, .TimelineValue = 0, .Semaphore = semaphore, .WaitStage = waitStage};
    }
    SemaphoreSubmit SemaphoreSubmit::Timeline(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 waitStage)
    {
        return SemaphoreSubmit{.SemaphoreType = VkSemaphoreType::VK_SEMAPHORE_TYPE_TIMELINE, .TimelineValue = value, .Semaphore = semaphore, .WaitStage = waitStage};
    }

    SemaphoreSubmit::operator VkSemaphoreSubmitInfo() const
    {
        return VkSemaphoreSubmitInfo
        {
            .sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = Semaphore,
            .value = TimelineValue,
            .stageMask = WaitStage
        };
    }

    DeviceCommandBuffer& DeviceCommandBuffer::AddWaitSemaphore(const SemaphoreSubmit& semaphore)
    {
        mWaitSemaphores.push_back(semaphore);
        return *this;
    }
    DeviceCommandBuffer& DeviceCommandBuffer::AddSignalSemaphore(const SemaphoreSubmit& semaphore)
    {
        mSignalSemaphores.push_back(semaphore);
        return *this;
    }

    void DeviceCommandBuffer::Submit(Queue queue)
    {
        if(!!queue.Queue)
        {
            mQueue = queue;
        }

        Assert(!!mQueue.Queue, "Cannot submit DeviceCommandBuffer without a queue!");
        Assert(!!mCommandBuffer, "Cannot submit uninitialized command buffer");

        if(mIsRecording)
        {
            End();
        }

        std::vector<VkSemaphoreSubmitInfo> signalSemaphores;
        std::vector<VkSemaphoreSubmitInfo> waitSemaphores;

        signalSemaphores.reserve(mSignalSemaphores.size());
        for (const SemaphoreSubmit& submit : mSignalSemaphores)
        {
            signalSemaphores.push_back(submit);
        }
        waitSemaphores.reserve(mWaitSemaphores.size());
        for (const SemaphoreSubmit& submit : mWaitSemaphores)
        {
            waitSemaphores.push_back(submit);
        }

        VkCommandBufferSubmitInfo bufferSubmitInfo{
            .sType         = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = mCommandBuffer,
        };

        VkSubmitInfo2 submitInfo{.sType                    = VkStructureType::VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                                 .waitSemaphoreInfoCount   = (uint32_t)waitSemaphores.size(),
                                 .pWaitSemaphoreInfos      = waitSemaphores.data(),
                                 .commandBufferInfoCount   = 1,
                                 .pCommandBufferInfos      = &bufferSubmitInfo,
                                 .signalSemaphoreInfoCount = (uint32_t)signalSemaphores.size(),
                                 .pSignalSemaphoreInfos    = signalSemaphores.data()};

        AssertVkResult(vkQueueSubmit2(mQueue, 1, &submitInfo, mFence));
    }

}  // namespace foray::core