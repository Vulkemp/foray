#include "foray_inflightframe.hpp"
#include "../core/foray_vkcontext.hpp"

namespace foray::base {

    void InFlightFrame::Create(const core::VkContext* context, uint32_t auxCommandBufferCount)
    {
        Destroy();
        mContext = context;
        mPrimaryCommandBuffer.Create(mContext);
        mPrimaryCommandBuffer.SetName(mContext, "Primary CommandBuffer");
        mAuxiliaryCommandBuffers.resize(auxCommandBufferCount);
        for(int32_t i = 0; i < auxCommandBufferCount; i++)
        {
            std::unique_ptr<core::DeviceCommandBuffer>& buf = mAuxiliaryCommandBuffers[i];
            buf = std::make_unique<core::DeviceCommandBuffer>();
            buf->Create(mContext);
            buf->SetName(mContext, fmt::format("Auxiliary CommandBuffer #{}", i));
        }

        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;

        AssertVkResult(vkCreateSemaphore(mContext->Device, &semaphoreCI, nullptr, &mSwapchainImageReady));

        AssertVkResult(vkCreateSemaphore(mContext->Device, &semaphoreCI, nullptr, &mPrimaryCompletedSemaphore));

        AssertVkResult(vkCreateFence(mContext->Device, &fenceCI, nullptr, &mPrimaryCompletedFence));
        mPrimaryCommandBuffer.SetFence(mPrimaryCompletedFence);
        mPrimaryCommandBuffer.AddSignalSemaphore(core::SemaphoreSubmit::Binary(mPrimaryCompletedSemaphore));
        if(auxCommandBufferCount == 0)
        {
            mPrimaryCommandBuffer.AddWaitSemaphore(core::SemaphoreSubmit::Binary(mSwapchainImageReady));
        }
    }

    void InFlightFrame::Destroy()
    {
        mPrimaryCommandBuffer.Destroy();
        mAuxiliaryCommandBuffers.resize(0);

        if(!!mSwapchainImageReady)
        {
            vkDestroySemaphore(mContext->Device, mSwapchainImageReady, nullptr);
        }
        if(!!mPrimaryCompletedSemaphore)
        {
            vkDestroySemaphore(mContext->Device, mPrimaryCompletedSemaphore, nullptr);
        }
        if(!!mPrimaryCompletedFence)
        {
            vkDestroyFence(mContext->Device, mPrimaryCompletedFence, nullptr);
        }
    }
    ESwapchainInteractResult InFlightFrame::AcquireSwapchainImage()
    {
        VkResult result = vkAcquireNextImageKHR(mContext->Device, mContext->Swapchain, UINT64_MAX, mSwapchainImageReady, nullptr, &mSwapchainImageIndex);

        if(result == VkResult::VK_ERROR_OUT_OF_DATE_KHR)
        {
            return ESwapchainInteractResult::Resized;
        }
        else if(result != VkResult::VK_SUBOPTIMAL_KHR)
        {
            AssertVkResult(result);
        }
        return ESwapchainInteractResult::Nominal;
    }
    ESwapchainInteractResult InFlightFrame::Present()
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &mPrimaryCompletedSemaphore;

        VkSwapchainKHR swapChains[] = {mContext->Swapchain};
        presentInfo.swapchainCount  = 1;
        presentInfo.pSwapchains     = swapChains;

        presentInfo.pImageIndices = &mSwapchainImageIndex;

        // Present on the present queue
        VkResult result = vkQueuePresentKHR(mContext->PresentQueue, &presentInfo);

        if(result == VkResult::VK_ERROR_OUT_OF_DATE_KHR || result == VkResult::VK_SUBOPTIMAL_KHR)
        {
            return ESwapchainInteractResult::Resized;
        }
        else
        {
            AssertVkResult(result);
        }
        return ESwapchainInteractResult::Nominal;
    }

    void InFlightFrame::ClearSwapchainImage(CmdBufferIndex index)
    {
        VkImage         swapchainImage = mContext->ContextSwapchain.SwapchainImages[mSwapchainImageIndex].Image;
        VkCommandBuffer cmdBuffer      = GetCommandBuffer(index);

        VkImageSubresourceRange range{};
        range.aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        VkImageMemoryBarrier barrier{};
        barrier.sType            = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.subresourceRange = range;

        barrier.srcAccessMask       = 0;
        barrier.dstAccessMask       = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = mContext->PresentQueue;
        barrier.dstQueueFamilyIndex = mContext->QueueGraphics;
        barrier.image               = swapchainImage;


        vkCmdPipelineBarrier(cmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

        // Clear swapchain image
        VkClearColorValue clearColor = VkClearColorValue{0.7f, 0.1f, 0.3f, 1.f};
        vkCmdClearColorImage(cmdBuffer, swapchainImage, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
    }

    core::DeviceCommandBuffer& InFlightFrame::GetAuxiliaryCommandBuffer(uint32_t index)
    {
        return *mAuxiliaryCommandBuffers[index];
    }
    core::DeviceCommandBuffer& InFlightFrame::GetPrimaryCommandBuffer()
    {
        return mPrimaryCommandBuffer;
    }
    core::DeviceCommandBuffer& InFlightFrame::GetCommandBuffer(CmdBufferIndex index)
    {
        if(index == PRIMARY_COMMAND_BUFFER)
        {
            return mPrimaryCommandBuffer;
        }
        return *mAuxiliaryCommandBuffers[index];
    }
    const core::DeviceCommandBuffer& InFlightFrame::GetAuxiliaryCommandBuffer(uint32_t index) const
    {
        return *mAuxiliaryCommandBuffers[index];
    }
    const core::DeviceCommandBuffer& InFlightFrame::GetPrimaryCommandBuffer() const
    {
        return mPrimaryCommandBuffer;
    }
    const core::DeviceCommandBuffer& InFlightFrame::GetCommandBuffer(CmdBufferIndex index) const
    {
        if(index == PRIMARY_COMMAND_BUFFER)
        {
            return mPrimaryCommandBuffer;
        }
        return *mAuxiliaryCommandBuffers[index];
    }

    bool InFlightFrame::HasFinishedExecution()
    {
        VkResult result = vkGetFenceStatus(mContext->Device, mPrimaryCompletedFence);
        if(result == VK_NOT_READY)
        {
            return false;
        }
        AssertVkResult(result);
        return true;
    }
    void InFlightFrame::WaitForExecutionFinished()
    {
        AssertVkResult(vkWaitForFences(mContext->Device, 1, &mPrimaryCompletedFence, VK_TRUE, UINT64_MAX));
    }

    void InFlightFrame::ResetFence()
    {
        AssertVkResult(vkResetFences(mContext->Device, 1, &mPrimaryCompletedFence));
    }

}  // namespace foray::base
