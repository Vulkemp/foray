#pragma once
#include "../core/imagelayoutcache.hpp"
#include "../basics.hpp"
#include "../vulkan.hpp"
#include "inflightframe.hpp"
#include "applifetime.hpp"
#include <vector>

namespace foray::base {

    /// @brief Context used for render processes. This object is rebuilt for every frame. 
    //// @remark Care should be taken if passed as value instead of reference because the ImageLayoutCache cannot work on multiple instances for a single render process
    class FrameRenderInfo
    {
      public:
        FORAY_PROPERTY_V(FrameTime)
        FORAY_PROPERTY_V(SinceStart)
        FORAY_PROPERTY_V(FrameNumber)
        FORAY_PROPERTY_V(RenderSize)
        FORAY_PROPERTY_R(InFlightFrame)
        FORAY_PROPERTY_R(ImageLayoutCache)

        inline core::DeviceSyncCommandBuffer&       GetAuxCommandBuffer(int32_t index) { return mInFlightFrame->GetAuxiliaryCommandBuffer(index); }
        inline const core::DeviceSyncCommandBuffer& GetAuxCommandBuffer(int32_t index) const { return mInFlightFrame->GetAuxiliaryCommandBuffer(index); }

        inline core::DeviceSyncCommandBuffer&       GetPrimaryCommandBuffer() { return mInFlightFrame->GetPrimaryCommandBuffer(); }
        inline const core::DeviceSyncCommandBuffer& GetPrimaryCommandBuffer() const { return mInFlightFrame->GetPrimaryCommandBuffer(); }

        inline core::DeviceSyncCommandBuffer&       GetCommandBuffer(CmdBufferIndex index) { return mInFlightFrame->GetCommandBuffer(index); }
        inline const core::DeviceSyncCommandBuffer& GetCommandBuffer(CmdBufferIndex index) const { return mInFlightFrame->GetCommandBuffer(index); }

        /// @brief Writes vkCmdClearColorImage cmd to the primary command buffer for the acquired image
        inline void ClearSwapchainImage(VkCommandBuffer cmdBuffer) { mInFlightFrame->ClearSwapchainImage(cmdBuffer, mImageLayoutCache); }
        /// @brief Adds a Pipeline barrier transitioning the swapchain image into present layout and assuring all writes to it have finished
        inline void PrepareSwapchainImageForPresent(VkCommandBuffer cmdBuffer) { mInFlightFrame->PrepareSwapchainImageForPresent(cmdBuffer, mImageLayoutCache); }
        /// @brief Batch submit all of InflightFrames command buffers at once
        inline void SubmitAllCommandBuffers() { mInFlightFrame->SubmitAll(); }

        FrameRenderInfo() = default;
        /// @brief Initialize based on loopRenderInfo and InFlightFrame
        /// @remark RenderLoop::RenderInfo::LoopFrameNumber is not used as mFrameNumber, 
        /// as the render index can be prevented from incrementing if the swapchain is resized before a frame could be recorded
        FrameRenderInfo(const LoopInfo& loopRenderInfo, InFlightFrame* inflightFrame)
            : mFrameTime(loopRenderInfo.Delta), mSinceStart(loopRenderInfo.SinceStart), mFrameNumber(0), mInFlightFrame(inflightFrame)
        {
        }

      protected:
        /// @brief Delta time in seconds since last frames render call
        fp32_t mFrameTime  = 0.f;
        /// @brief Delta time in seconds since application start
        fp64_t mSinceStart = 0.0;
        /// @brief Number of complete frames rendered since application startup
        uint64_t mFrameNumber = 0;
        /// @brief Render Resolution
        VkExtent2D mRenderSize = {};
        /// @brief In Flight Frame contains command buffers and synchronization primitives for the current frame
        InFlightFrame*         mInFlightFrame = nullptr;
        /// @brief ImageLayoutCache is used to maintain Images layouts throughout the recording of the frame.
        /// @remark This is necessary, because Image Layout Transitions need the old ImageLayout set, otherwise the driver is free
        /// to discard the Image's content at will (e.g. NVidia GTX 10XX series drivers)
        core::ImageLayoutCache mImageLayoutCache;
    };
}  // namespace foray::base