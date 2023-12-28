#include "commandbuffer.hpp"
#include "../vulkan.hpp"

namespace foray::core {

    CommandBuffer::CommandBuffer(Context* context, VkCommandBufferLevel cmdBufferLvl, bool begin)
      : mContext(context)
    {
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool        = mContext->CommandPool;
        cmdBufAllocateInfo.level              = cmdBufferLvl;
        cmdBufAllocateInfo.commandBufferCount = 1;

        AssertVkResult(mContext->DispatchTable().allocateCommandBuffers(&cmdBufAllocateInfo, &mCommandBuffer));

        // If requested, also start recording for the new command buffer
        if(begin)
        {
            Begin();
        }
    }

    void CommandBuffer::Begin()
    {
        VkCommandBufferBeginInfo commandBufferBI{};
        commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        AssertVkResult(mContext->DispatchTable().beginCommandBuffer(mCommandBuffer, &commandBufferBI));
        mIsRecording = true;
    }

    void CommandBuffer::End()
    {
        AssertVkResult(mContext->DispatchTable().endCommandBuffer(mCommandBuffer));
        mIsRecording = false;
    }

    void CommandBuffer::Reset(VkCommandBufferResetFlags flags)
    {
        AssertVkResult(mContext->DispatchTable().resetCommandBuffer(mCommandBuffer, flags));
    }

    void CommandBuffer::SetName(std::string_view name)
    {
        SetObjectName(mContext, mCommandBuffer, name, true);
    }

    CommandBuffer::~CommandBuffer()
    {
        if(!!mContext && !!mCommandBuffer)
        {
            mContext->DispatchTable().freeCommandBuffers(mContext->CommandPool, 1, &mCommandBuffer);
            mCommandBuffer = nullptr;
            mContext = nullptr;
        }
    }

    /// @brief Create based on the contexts device, command pool and queue.
    HostSyncCommandBuffer::HostSyncCommandBuffer(Context* context, VkCommandBufferLevel cmdBufferLvl, bool begin)
       : CommandBuffer(context, cmdBufferLvl, begin)
    {

        VkFenceCreateInfo fenceCi{.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        mContext->DispatchTable().createFence(&fenceCi, nullptr, &mFence);
    }

    void HostSyncCommandBuffer::Submit()
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
        AssertVkResult(mContext->DispatchTable().queueSubmit(mContext->Queue, 1, &submitInfo, mFence));
    }
    void HostSyncCommandBuffer::SubmitAndWait()
    {
        Submit();
        WaitForCompletion();
    }
    bool HostSyncCommandBuffer::HasCompleted()
    {
        vk::Result result = mContext->DispatchTable().getFenceStatus(mFence);
        if(result == VK_NOT_READY)
        {
            return false;
        }
        AssertVkResult(result);
        return true;
    }
    void HostSyncCommandBuffer::WaitForCompletion()
    {
        // Wait for the fence to signal that command buffer has finished executing
        AssertVkResult(mContext->DispatchTable().waitForFences(1, &mFence, VK_TRUE, UINT64_MAX));
        AssertVkResult(mContext->DispatchTable().resetFences(1, &mFence));
    }

    HostSyncCommandBuffer::~HostSyncCommandBuffer()
    {
        if(!!mContext && !!mFence)
        {
            mContext->DispatchTable().destroyFence(mFence, nullptr);
        }
    }

    SemaphoreReference SemaphoreReference::Binary(VkSemaphore semaphore, vk::PipelineStageFlags2 waitStage)
    {
        return SemaphoreReference{.SemaphoreType = VkSemaphoreType::VK_SEMAPHORE_TYPE_BINARY, .TimelineValue = 0, .Semaphore = semaphore, .WaitStage = waitStage};
    }
    SemaphoreReference SemaphoreReference::Timeline(VkSemaphore semaphore, uint64_t value, vk::PipelineStageFlags2 waitStage)
    {
        return SemaphoreReference{.SemaphoreType = VkSemaphoreType::VK_SEMAPHORE_TYPE_TIMELINE, .TimelineValue = value, .Semaphore = semaphore, .WaitStage = waitStage};
    }

    SemaphoreReference::operator VkSemaphoreSubmitInfo() const
    {
        return VkSemaphoreSubmitInfo{.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .semaphore = Semaphore, .value = TimelineValue, .stageMask = WaitStage};
    }

    DeviceSyncCommandBuffer& DeviceSyncCommandBuffer::AddWaitSemaphore(const SemaphoreReference& semaphore)
    {
        mWaitSemaphores.push_back(semaphore);
        return *this;
    }
    DeviceSyncCommandBuffer& DeviceSyncCommandBuffer::AddSignalSemaphore(const SemaphoreReference& semaphore)
    {
        mSignalSemaphores.push_back(semaphore);
        return *this;
    }

    void DeviceSyncCommandBuffer::Submit()
    {
        Assert(!!mCommandBuffer, "Cannot submit uninitialized command buffer");

        if(mIsRecording)
        {
            End();
        }

        std::vector<VkSemaphoreSubmitInfo> signalSemaphores;
        std::vector<VkSemaphoreSubmitInfo> waitSemaphores;

        signalSemaphores.reserve(mSignalSemaphores.size());
        for(const SemaphoreReference& submit : mSignalSemaphores)
        {
            signalSemaphores.push_back(submit);
        }
        waitSemaphores.reserve(mWaitSemaphores.size());
        for(const SemaphoreReference& submit : mWaitSemaphores)
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

        AssertVkResult(mContext->DispatchTable().queueSubmit2(mContext->Queue, 1, &submitInfo, mFence));
    }

    void DeviceSyncCommandBuffer::WriteToSubmitInfo(std::vector<VkSubmitInfo2>& submitInfos)
    {
        Assert(!!mCommandBuffer, "Cannot submit uninitialized command buffer");

        if(mIsRecording)
        {
            End();
        }

        std::vector<VkSemaphoreSubmitInfo> signalSemaphores;
        std::vector<VkSemaphoreSubmitInfo> waitSemaphores;

        signalSemaphores.reserve(mSignalSemaphores.size());
        for(const SemaphoreReference& submit : mSignalSemaphores)
        {
            signalSemaphores.push_back(submit);
        }
        waitSemaphores.reserve(mWaitSemaphores.size());
        for(const SemaphoreReference& submit : mWaitSemaphores)
        {
            waitSemaphores.push_back(submit);
        }

        VkCommandBufferSubmitInfo bufferSubmitInfo{
            .sType         = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = mCommandBuffer,
        };

        submitInfos.push_back(VkSubmitInfo2{.sType                    = VkStructureType::VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                                            .waitSemaphoreInfoCount   = (uint32_t)waitSemaphores.size(),
                                            .pWaitSemaphoreInfos      = waitSemaphores.data(),
                                            .commandBufferInfoCount   = 1,
                                            .pCommandBufferInfos      = &bufferSubmitInfo,
                                            .signalSemaphoreInfoCount = (uint32_t)signalSemaphores.size(),
                                            .pSignalSemaphoreInfos    = signalSemaphores.data()});
    }
}  // namespace foray::core