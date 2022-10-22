#pragma once
#include "../core/foray_imagelayoutcache.hpp"
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include "foray_inflightframe.hpp"
#include "foray_renderloop.hpp"
#include <vector>

namespace foray::base {

    class FrameRenderInfo
    {
      public:
        FORAY_PROPERTY_ALL(FrameTime)
        FORAY_PROPERTY_ALL(SinceStart)
        FORAY_PROPERTY_ALL(FrameNumber)
        FORAY_PROPERTY_ALL(InFlightFrame)
        FORAY_PROPERTY_ALL(ImageLayoutCache)

        inline core::DeviceCommandBuffer&       GetAuxCommandBuffer(int32_t index) { return mInFlightFrame->GetAuxiliaryCommandBuffer(index); }
        inline const core::DeviceCommandBuffer& GetAuxCommandBuffer(int32_t index) const { return mInFlightFrame->GetAuxiliaryCommandBuffer(index); }

        inline core::DeviceCommandBuffer&       GetPrimaryCommandBuffer() { return mInFlightFrame->GetPrimaryCommandBuffer(); }
        inline const core::DeviceCommandBuffer& GetPrimaryCommandBuffer() const { return mInFlightFrame->GetPrimaryCommandBuffer(); }

        inline core::DeviceCommandBuffer&       GetCommandBuffer(CmdBufferIndex index) { return mInFlightFrame->GetCommandBuffer(index); }
        inline const core::DeviceCommandBuffer& GetCommandBuffer(CmdBufferIndex index) const { return mInFlightFrame->GetCommandBuffer(index); }

        /// @brief Writes vkCmdClearColorImage cmd to the primary command buffer for the acquired image
        inline void ClearSwapchainImage(VkCommandBuffer cmdBuffer) { mInFlightFrame->ClearSwapchainImage(cmdBuffer, mImageLayoutCache); }
        /// @brief Adds a Pipeline barrier transitioning the swapchain image into present layout and assuring all writes to it have finished
        inline void PrepareSwapchainImageForPresent(VkCommandBuffer cmdBuffer) { mInFlightFrame->PrepareSwapchainImageForPresent(cmdBuffer, mImageLayoutCache); }

        FrameRenderInfo() = default;
        FrameRenderInfo(const RenderLoop::RenderInfo& loopRenderInfo, InFlightFrame* inflightFrame)
            : mFrameTime(loopRenderInfo.Delta), mSinceStart(loopRenderInfo.SinceStart), mFrameNumber(0), mInFlightFrame(inflightFrame)
        {
        }

      protected:
        fp32_t mFrameTime  = 0.f;
        fp64_t mSinceStart = 0.0;
        /// @brief Number of complete frames rendered since application startup
        uint64_t mFrameNumber = 0;


        InFlightFrame*         mInFlightFrame = nullptr;
        core::ImageLayoutCache mImageLayoutCache;
    };
}  // namespace foray::base