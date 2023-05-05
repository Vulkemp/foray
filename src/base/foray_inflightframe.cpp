#include "foray_inflightframe.hpp"
#include "../core/foray_context.hpp"
#include "../core/foray_imagelayoutcache.hpp"


namespace foray::base {

    InFlightFrame::InFlightFrame(core::Context* context, uint32_t auxCommandBufferCount)
    {
        mContext = context;
        Assert(!!mContext->Device, "[InFlightFrame::Create] Requires Dispatch table");
        mPrimaryCommandBuffer.New(context);
        mPrimaryCommandBuffer->SetName("Primary CommandBuffer");
        mAuxiliaryCommandBuffers.resize(auxCommandBufferCount);
        for(int32_t i = 0; i < (int32_t)auxCommandBufferCount; i++)
        {
            Local<core::DeviceSyncCommandBuffer>& buf = mAuxiliaryCommandBuffers[i];
            buf.New(context);
            buf->SetName(fmt::format("Auxiliary CommandBuffer #{}", i));
        }

        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;

        AssertVkResult(mContext->DispatchTable().createSemaphore(&semaphoreCI, nullptr, &mSwapchainImageReady));

        AssertVkResult(mContext->DispatchTable().createSemaphore(&semaphoreCI, nullptr, &mPrimaryCompletedSemaphore));

        AssertVkResult(mContext->DispatchTable().createFence(&fenceCI, nullptr, &mPrimaryCompletedFence));
        mPrimaryCommandBuffer->SetFence(mPrimaryCompletedFence);
        mPrimaryCommandBuffer->AddSignalSemaphore(core::SemaphoreReference::Binary(mPrimaryCompletedSemaphore));
        if(auxCommandBufferCount == 0)
        {
            mPrimaryCommandBuffer->AddWaitSemaphore(core::SemaphoreReference::Binary(mSwapchainImageReady));
        }
    }

    InFlightFrame::~InFlightFrame()
    {
        if(!!mSwapchainImageReady)
        {
            mContext->DispatchTable().destroySemaphore(mSwapchainImageReady, nullptr);
        }
        if(!!mPrimaryCompletedSemaphore)
        {
            mContext->DispatchTable().destroySemaphore(mPrimaryCompletedSemaphore, nullptr);
        }
        if(!!mPrimaryCompletedFence)
        {
            mContext->DispatchTable().destroyFence(mPrimaryCompletedFence, nullptr);
        }
    }
    ESwapchainInteractResult InFlightFrame::AcquireSwapchainImage()
    {
        Assert(!!(mContext->Device), "[InFlightFrame::AcquireSwapchainImage] Requires Dispatch Table");
        Assert(!!(mContext->WindowSwapchain), "[InFlightFrame::AcquireSwapchainImage] Requires Swapchain");
        VkSwapchainKHR swapchain = *mContext->WindowSwapchain;
        VkResult       result    = mContext->DispatchTable().acquireNextImageKHR(swapchain, UINT64_MAX, mSwapchainImageReady, nullptr, &mSwapchainImageIndex);

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
        Assert(!!(mContext->Device), "[InFlightFrame::AcquireSwapchainImage] Requires Dispatch Table");
        Assert(!!(mContext->WindowSwapchain), "[InFlightFrame::AcquireSwapchainImage] Requires Swapchain");

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &mPrimaryCompletedSemaphore;

        VkSwapchainKHR swapChains[] = {*mContext->WindowSwapchain};
        presentInfo.swapchainCount  = 1;
        presentInfo.pSwapchains     = swapChains;

        presentInfo.pImageIndices = &mSwapchainImageIndex;

        // Present on the present queue
        VkResult result = mContext->DispatchTable().queuePresentKHR(mContext->Queue, &presentInfo);

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
        const core::SwapchainImageInfo& swapchainImage = mContext->WindowSwapchain->GetSwapchainImages()[mSwapchainImageIndex];

        core::ImageLayoutCache::Barrier2 barrier{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                 .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                 .DstStageMask  = VK_PIPELINE_STAGE_2_NONE,
                                                 .DstAccessMask = 0,
                                                 .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
        VkImageMemoryBarrier2            vkBarrier = imgLayoutCache.MakeBarrier(swapchainImage.Image, barrier);

        VkDependencyInfo depInfo{
            .sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
            .imageMemoryBarrierCount = 1U,
            .pImageMemoryBarriers    = &vkBarrier,
        };

        mContext->DispatchTable().cmdPipelineBarrier2(cmdBuffer, &depInfo);
    }

    void InFlightFrame::ClearSwapchainImage(VkCommandBuffer cmdBuffer, core::ImageLayoutCache& imgLayoutCache)
    {
        const core::SwapchainImageInfo& swapchainImage = mContext->WindowSwapchain->GetSwapchainImages()[mSwapchainImageIndex];

        core::ImageLayoutCache::Barrier2 barrier{
            .SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .SrcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
            .DstStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .DstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        };
        VkImageMemoryBarrier2 vkBarrier = imgLayoutCache.MakeBarrier(swapchainImage.Image, barrier);

        VkDependencyInfo depInfo{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                 .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
                                 .imageMemoryBarrierCount = 1U,
                                 .pImageMemoryBarriers    = &vkBarrier};

        mContext->DispatchTable().cmdPipelineBarrier2(cmdBuffer, &depInfo);

        VkImageSubresourceRange range{
            .aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        };

        // Clear swapchain image
        VkClearColorValue clearColor = VkClearColorValue{{0.7f, 0.1f, 0.3f, 1.f}};
        mContext->DispatchTable().cmdClearColorImage(cmdBuffer, swapchainImage, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
    }

    core::DeviceSyncCommandBuffer& InFlightFrame::GetAuxiliaryCommandBuffer(uint32_t index)
    {
        return mAuxiliaryCommandBuffers[index].GetRef();
    }
    core::DeviceSyncCommandBuffer& InFlightFrame::GetPrimaryCommandBuffer()
    {
        return mPrimaryCommandBuffer.GetRef();
    }
    core::DeviceSyncCommandBuffer& InFlightFrame::GetCommandBuffer(CmdBufferIndex index)
    {
        if(index == PRIMARY_COMMAND_BUFFER)
        {
            return mPrimaryCommandBuffer.GetRef();
        }
        return mAuxiliaryCommandBuffers[index].GetRef();
    }
    const core::DeviceSyncCommandBuffer& InFlightFrame::GetAuxiliaryCommandBuffer(uint32_t index) const
    {
        return mAuxiliaryCommandBuffers[index].GetRef();
    }
    const core::DeviceSyncCommandBuffer& InFlightFrame::GetPrimaryCommandBuffer() const
    {
        return mPrimaryCommandBuffer.GetRef();
    }
    const core::DeviceSyncCommandBuffer& InFlightFrame::GetCommandBuffer(CmdBufferIndex index) const
    {
        if(index == PRIMARY_COMMAND_BUFFER)
        {
            return mPrimaryCommandBuffer.GetRef();
        }
        return mAuxiliaryCommandBuffers[index].GetRef();
    }

    bool InFlightFrame::HasFinishedExecution()
    {
        VkResult result = mContext->DispatchTable().getFenceStatus(mPrimaryCompletedFence);
        if(result == VK_NOT_READY)
        {
            return false;
        }
        AssertVkResult(result);
        return true;
    }
    void InFlightFrame::WaitForExecutionFinished()
    {
        AssertVkResult(mContext->DispatchTable().waitForFences(1, &mPrimaryCompletedFence, VK_TRUE, UINT64_MAX));
    }

    void InFlightFrame::ResetFence()
    {
        AssertVkResult(mContext->DispatchTable().resetFences(1, &mPrimaryCompletedFence));
    }

    void InFlightFrame::SubmitAll()
    {
        std::vector<VkSubmitInfo2> submitInfos;
        mPrimaryCommandBuffer->WriteToSubmitInfo(submitInfos);
        for (Local<core::DeviceSyncCommandBuffer>& auxCommandBuffer : mAuxiliaryCommandBuffers)
        {
            auxCommandBuffer->WriteToSubmitInfo(submitInfos);
        }
        mContext->DispatchTable().queueSubmit2(mContext->Queue, (uint32_t)submitInfos.size(), submitInfos.data(), mPrimaryCompletedFence);
    }

}  // namespace foray::base
