#include "foray_inflightframe.hpp"
#include "../core/foray_context.hpp"
#include "../core/foray_imagelayoutcache.hpp"


namespace foray::base {

    void InFlightFrame::Create(core::Context* context, uint32_t auxCommandBufferCount)
    {
        Destroy();
        mContext = context;
        Assert(!!mContext->VkbDispatchTable, "[InFlightFrame::Create] Requires Dispatch table");
        mPrimaryCommandBuffer.Create(context);
        mPrimaryCommandBuffer.SetName(context, "Primary CommandBuffer");
        mAuxiliaryCommandBuffers.resize(auxCommandBufferCount);
        for(int32_t i = 0; i < auxCommandBufferCount; i++)
        {
            std::unique_ptr<core::DeviceCommandBuffer>& buf = mAuxiliaryCommandBuffers[i];
            buf                                             = std::make_unique<core::DeviceCommandBuffer>();
            buf->Create(context);
            buf->SetName(context, fmt::format("Auxiliary CommandBuffer #{}", i));
        }

        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;

        AssertVkResult(mContext->VkbDispatchTable->createSemaphore(&semaphoreCI, nullptr, &mSwapchainImageReady));

        AssertVkResult(mContext->VkbDispatchTable->createSemaphore(&semaphoreCI, nullptr, &mPrimaryCompletedSemaphore));

        AssertVkResult(mContext->VkbDispatchTable->createFence(&fenceCI, nullptr, &mPrimaryCompletedFence));
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
            mContext->VkbDispatchTable->destroySemaphore(mSwapchainImageReady, nullptr);
            mSwapchainImageReady = nullptr;
        }
        if(!!mPrimaryCompletedSemaphore)
        {
            mContext->VkbDispatchTable->destroySemaphore(mPrimaryCompletedSemaphore, nullptr);
            mPrimaryCompletedSemaphore = nullptr;
        }
        if(!!mPrimaryCompletedFence)
        {
            mContext->VkbDispatchTable->destroyFence(mPrimaryCompletedFence, nullptr);
            mPrimaryCompletedFence = nullptr;
        }
        mContext = nullptr;
    }
    ESwapchainInteractResult InFlightFrame::AcquireSwapchainImage()
    {
        Assert(!!(mContext->VkbDispatchTable), "[InFlightFrame::AcquireSwapchainImage] Requires Dispatch Table");
        Assert(!!(mContext->Swapchain), "[InFlightFrame::AcquireSwapchainImage] Requires Swapchain");
        VkSwapchainKHR swapchain = mContext->Swapchain->swapchain;
        VkResult       result    = mContext->VkbDispatchTable->acquireNextImageKHR(swapchain, UINT64_MAX, mSwapchainImageReady, nullptr, &mSwapchainImageIndex);

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
        Assert(!!(mContext->VkbDispatchTable), "[InFlightFrame::AcquireSwapchainImage] Requires Dispatch Table");
        Assert(!!(mContext->Swapchain), "[InFlightFrame::AcquireSwapchainImage] Requires Swapchain");

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &mPrimaryCompletedSemaphore;

        VkSwapchainKHR swapChains[] = {mContext->Swapchain->swapchain};
        presentInfo.swapchainCount  = 1;
        presentInfo.pSwapchains     = swapChains;

        presentInfo.pImageIndices = &mSwapchainImageIndex;

        // Present on the present queue
        VkResult result = mContext->VkbDispatchTable->queuePresentKHR(mContext->Queue, &presentInfo);

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

    void InFlightFrame::PrepareSwapchainImageForPresent(VkCommandBuffer cmdBuffer, core::ImageLayoutCache& imgLayoutCache)
    {
        const core::SwapchainImageInfo& swapchainImage = mContext->SwapchainImages[mSwapchainImageIndex];

        core::ImageLayoutCache::Barrier2 barrier{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                 .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                 .DstStageMask  = VK_PIPELINE_STAGE_2_NONE,
                                                 .DstAccessMask = 0,
                                                 .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
        VkImageMemoryBarrier2            vkBarrier = imgLayoutCache.Set(swapchainImage.Name, swapchainImage.Image, barrier);

        VkDependencyInfo depInfo{
            .sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
            .imageMemoryBarrierCount = 1U,
            .pImageMemoryBarriers    = &vkBarrier,
        };

        mContext->VkbDispatchTable->cmdPipelineBarrier2(cmdBuffer, &depInfo);
    }

    void InFlightFrame::ClearSwapchainImage(VkCommandBuffer cmdBuffer, core::ImageLayoutCache& imgLayoutCache)
    {
        const core::SwapchainImageInfo& swapchainImage = mContext->SwapchainImages[mSwapchainImageIndex];

        core::ImageLayoutCache::Barrier2 barrier{
            .SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .SrcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
            .DstStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .DstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        };
        VkImageMemoryBarrier2 vkBarrier = imgLayoutCache.Set(swapchainImage.Name, swapchainImage.Image, barrier);

        VkDependencyInfo depInfo{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                 .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
                                 .imageMemoryBarrierCount = 1U,
                                 .pImageMemoryBarriers    = &vkBarrier};

        mContext->VkbDispatchTable->cmdPipelineBarrier2(cmdBuffer, &depInfo);

        VkImageSubresourceRange range{
            .aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        };

        // Clear swapchain image
        VkClearColorValue clearColor = VkClearColorValue{0.7f, 0.1f, 0.3f, 1.f};
        mContext->VkbDispatchTable->cmdClearColorImage(cmdBuffer, swapchainImage, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
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
        VkResult result = mContext->VkbDispatchTable->getFenceStatus(mPrimaryCompletedFence);
        if(result == VK_NOT_READY)
        {
            return false;
        }
        AssertVkResult(result);
        return true;
    }
    void InFlightFrame::WaitForExecutionFinished()
    {
        AssertVkResult(mContext->VkbDispatchTable->waitForFences(1, &mPrimaryCompletedFence, VK_TRUE, UINT64_MAX));
    }

    void InFlightFrame::ResetFence()
    {
        AssertVkResult(mContext->VkbDispatchTable->resetFences(1, &mPrimaryCompletedFence));
    }

}  // namespace foray::base
