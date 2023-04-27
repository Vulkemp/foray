#pragma once
#include "../core/foray_commandbuffer.hpp"
#include "../core/foray_core_declares.hpp"
#include <vector>

namespace foray::base {

    /// @brief Result of swapchain interaction (AcquireImage or Present)
    enum class ESwapchainInteractResult
    {
        /// @brief The interaction resulted nominally
        Nominal,
        /// @brief The interaction indicated that the swapchain should be resized
        Resized
    };

    using CmdBufferIndex                               = int32_t;
    static const CmdBufferIndex PRIMARY_COMMAND_BUFFER = -1;

    /// @brief Wraps synchronization primitives and command buffers for an inflight frame
    /// @details
    /// USAGE (SINGLE CMD BUFFER PER FRAME):
    ///   - Use the primary command buffer. Submit it before presenting
    /// 
    /// USAGE (MULTIPLE)
    ///   - The Primary command buffer should be used for swapchain interaction aswell as host synchronization
    ///     - InflightFrame's methods respect this
    ///   - The user must synchronize all commandbuffers themselves, so the primary commandbuffer functions properly:
    ///     - Add the SwapchainImageReady Semaphore as a waitsemaphore to one of the command buffers
    ///     - Use semaphores to synchronise your command buffers as needed
    class InFlightFrame
    {
      public:
        InFlightFrame(core::Context* context, uint32_t auxCommandBufferCount = 0);

        virtual ~InFlightFrame();

        /// @brief Acquires next swapchain image and stores the resulting index in mSwapchainImageIndex
        /// @return If result is Resized, the swapchain must be resized immediately
        ESwapchainInteractResult AcquireSwapchainImage();
        /// @brief Presents the previously acquired image. The primary command buffer must have been submitted prior to this call!
        /// @return If result is Resized, the swapchain must be resized
        ESwapchainInteractResult Present();

        /// @brief Writes vkCmdClearColorImage cmd to the primary command buffer for the acquired image
        void ClearSwapchainImage(VkCommandBuffer cmdBuffer, core::ImageLayoutCache& imgLayoutCache);
        /// @brief Adds a Pipeline barrier transitioning the swapchain image into present layout and assuring all writes to it have finished
        void PrepareSwapchainImageForPresent(VkCommandBuffer cmdBuffer, core::ImageLayoutCache& imgLayoutCache);

        /// @brief Non-blocking check wether the frame has been finished execution
        bool HasFinishedExecution();
        /// @brief Blocks the current thread until the frame has finished execution
        void WaitForExecutionFinished();
        /// @brief Resets the frames host synchronization fence
        void ResetFence();

        /// @brief Submits all command buffers by getting all VkSubmitInfo2{} structures from DeviceSyncCommandBuffer::WriteToSubmitInfo(...) 
        /// and submitting them in a single vkQueueSubmit2(...) call
        void SubmitAll();

        /// @brief Get an auxiliary command buffer
        /// @param index Command buffer index to return
        core::DeviceSyncCommandBuffer&       GetAuxiliaryCommandBuffer(uint32_t index);
        /// @brief Get primary command buffer
        core::DeviceSyncCommandBuffer&       GetPrimaryCommandBuffer();
        /// @brief Get a command buffer
        /// @param index If PRIMARY_COMMAND_BUFFER, the primary is returned. Auxiliary command buffer index otherwise
        core::DeviceSyncCommandBuffer&       GetCommandBuffer(CmdBufferIndex index);
        /// @brief Get an auxiliary command buffer
        /// @param index Command buffer index to return
        const core::DeviceSyncCommandBuffer& GetAuxiliaryCommandBuffer(uint32_t index) const;
        /// @brief Get primary command buffer
        const core::DeviceSyncCommandBuffer& GetPrimaryCommandBuffer() const;
        /// @brief Get a command buffer
        /// @param index If PRIMARY_COMMAND_BUFFER, the primary is returned. Auxiliary command buffer index otherwise
        const core::DeviceSyncCommandBuffer& GetCommandBuffer(CmdBufferIndex index) const;

        FORAY_GETTER_V(SwapchainImageIndex)
        FORAY_GETTER_V(SwapchainImageReady)
        FORAY_GETTER_V(PrimaryCompletedSemaphore)
        FORAY_GETTER_V(PrimaryCompletedFence)

      protected:
        core::Context* mContext = nullptr;

        /// @brief Auxiliary command buffers
        std::vector<std::unique_ptr<core::DeviceSyncCommandBuffer>> mAuxiliaryCommandBuffers;
        /// @brief Primary command buffer
        core::DeviceSyncCommandBuffer                               mPrimaryCommandBuffer;
        /// @brief Semaphore signalled by the device when the swapchain image becomes ready
        VkSemaphore mSwapchainImageReady       = nullptr;
        /// @brief Semaphore signalled by the primary command buffer when execution has finished
        VkSemaphore mPrimaryCompletedSemaphore = nullptr;
        /// @brief Fence signalled after the primary / all command buffers have finished execution
        VkFence     mPrimaryCompletedFence     = nullptr;

        uint32_t mSwapchainImageIndex = 0;
    };
}  // namespace foray::base
