#include "foray_commandbuffer.hpp"
#include "../foray_vulkan.hpp"

namespace foray::core {

    VkCommandBuffer CommandBuffer::Create(Context* context, VkCommandBufferLevel cmdBufferLvl, bool begin)
    {
        mContext = context;
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool        = mContext->CommandPool;
        cmdBufAllocateInfo.level              = cmdBufferLvl;
        cmdBufAllocateInfo.commandBufferCount = 1;

        AssertVkResult(mContext->VkbDispatchTable->allocateCommandBuffers(&cmdBufAllocateInfo, &mCommandBuffer));

        // If requested, also start recording for the new command buffer
        if(begin)
        {
            Begin();
        }

        return mCommandBuffer;
    }

    void CommandBuffer::Begin()
    {
        VkCommandBufferBeginInfo commandBufferBI{};
        commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        AssertVkResult(mContext->VkbDispatchTable->beginCommandBuffer(mCommandBuffer, &commandBufferBI));
        mIsRecording = true;
    }

    void CommandBuffer::End()
    {
        AssertVkResult(mContext->VkbDispatchTable->endCommandBuffer(mCommandBuffer));
        mIsRecording = false;
    }

    void CommandBuffer::Reset(VkCommandBufferResetFlags flags)
    {
        AssertVkResult(mContext->VkbDispatchTable->resetCommandBuffer(mCommandBuffer, flags));
    }

    void CommandBuffer::SetName(std::string_view name)
    {
        SetObjectName(mContext, mCommandBuffer, name, true);
    }

    void CommandBuffer::Destroy()
    {
        if(!!mCommandBuffer)
        {
            mContext->VkbDispatchTable->freeCommandBuffers(mContext->CommandPool, 1, &mCommandBuffer);
            mCommandBuffer = nullptr;
        }
    }

    /// @brief Create based on the contexts device, command pool and queue.
    VkCommandBuffer HostCommandBuffer::Create(Context* context, VkCommandBufferLevel cmdBufferLvl, bool begin)
    {
        CommandBuffer::Create(context, cmdBufferLvl, begin);

        VkFenceCreateInfo fenceCi{.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        mContext->VkbDispatchTable->createFence(&fenceCi, nullptr, &mFence);

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
        AssertVkResult(mContext->VkbDispatchTable->queueSubmit(mContext->Queue, 1, &submitInfo, mFence));
    }
    void HostCommandBuffer::SubmitAndWait()
    {
        Submit();
        WaitForCompletion();
    }
    bool HostCommandBuffer::HasCompleted()
    {
        VkResult result = mContext->VkbDispatchTable->getFenceStatus(mFence);
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
        AssertVkResult(mContext->VkbDispatchTable->waitForFences(1, &mFence, VK_TRUE, UINT64_MAX));
        AssertVkResult(mContext->VkbDispatchTable->resetFences(1, &mFence));
    }

    void HostCommandBuffer::Destroy()
    {
        if(!!mFence)
        {
            mContext->VkbDispatchTable->destroyFence(mFence, nullptr);
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
        return VkSemaphoreSubmitInfo{.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .semaphore = Semaphore, .value = TimelineValue, .stageMask = WaitStage};
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

    void DeviceCommandBuffer::Submit()
    {
        Assert(!!mCommandBuffer, "Cannot submit uninitialized command buffer");

        if(mIsRecording)
        {
            End();
        }

        std::vector<VkSemaphoreSubmitInfo> signalSemaphores;
        std::vector<VkSemaphoreSubmitInfo> waitSemaphores;

        signalSemaphores.reserve(mSignalSemaphores.size());
        for(const SemaphoreSubmit& submit : mSignalSemaphores)
        {
            signalSemaphores.push_back(submit);
        }
        waitSemaphores.reserve(mWaitSemaphores.size());
        for(const SemaphoreSubmit& submit : mWaitSemaphores)
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

        AssertVkResult(mContext->VkbDispatchTable->queueSubmit2(mContext->Queue, 1, &submitInfo, mFence));
    }

}  // namespace foray::core