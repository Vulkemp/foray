#pragma once
#include "../core/foray_commandbuffer.hpp"
#include <vector>

namespace foray::base {


    enum class ESwapchainInteractResult
    {
        Nominal,
        Resized
    };

    using CmdBufferIndex                               = int32_t;
    static const CmdBufferIndex PRIMARY_COMMAND_BUFFER = -1;

    /// @brief Wraps synchronization primitives and command buffers for an inflight frame
    class InFlightFrame
    {
        /*
        USAGE (SINGLE CMD BUFFER PER FRAME):
          - Use the primary command buffer. Submit it before presenting
        
        USAGE (MULTIPLE)
          - The Primary command buffer should be used for swapchain interaction aswell as host synchronization
            - InflightFrame's methods respect this
          - The user must synchronize all commandbuffers themselves, so the primary commandbuffer functions properly:
            - Add the SwapchainImageReady Semaphore as a waitsemaphore to one of the command buffers
            - Use semaphores to synchronise your command buffers as needed
      */
      public:
        void Create(const core::VkContext* context, uint32_t auxCommandBufferCount = 0);
        void Destroy();

        /// @brief Acquires next swapchain image and stores the resulting index in mSwapchainImageIndex
        /// @return If result is Resized, the swapchain must be resized immediately
        ESwapchainInteractResult AcquireSwapchainImage();
        /// @brief Presents the previously acquired image. The primary command buffer must have been submitted prior to this call!
        /// @return If result is Resized, the swapchain must be resized
        ESwapchainInteractResult Present();

        /// @brief Writes vkCmdClearColorImage cmd to the primary command buffer for the acquired image
        void ClearSwapchainImage(CmdBufferIndex index = PRIMARY_COMMAND_BUFFER);

        /// @brief Non-blocking check wether the frame has been finished execution
        bool HasFinishedExecution();
        /// @brief Blocks the current thread until the frame has finished execution
        void WaitForExecutionFinished();
        void ResetFence();

        /// @brief Get an auxiliary command buffer
        /// @param index Command buffer index to return
        core::DeviceCommandBuffer&       GetAuxiliaryCommandBuffer(uint32_t index);
        core::DeviceCommandBuffer&       GetPrimaryCommandBuffer();
        core::DeviceCommandBuffer&       GetCommandBuffer(CmdBufferIndex index);
        const core::DeviceCommandBuffer& GetAuxiliaryCommandBuffer(uint32_t index) const;
        const core::DeviceCommandBuffer& GetPrimaryCommandBuffer() const;
        const core::DeviceCommandBuffer& GetCommandBuffer(CmdBufferIndex index) const;

        FORAY_PROPERTY_CGET(SwapchainImageIndex)
        FORAY_PROPERTY_ALLGET(SwapchainImageReady)
        FORAY_PROPERTY_ALLGET(PrimaryCompletedSemaphore)
        FORAY_PROPERTY_ALLGET(PrimaryCompletedFence)

      protected:
        const core::VkContext* mContext = nullptr;

        /// @brief All command buffers used by the frame
        std::vector<std::unique_ptr<core::DeviceCommandBuffer>> mAuxiliaryCommandBuffers;
        core::DeviceCommandBuffer                               mPrimaryCommandBuffer;

        VkSemaphore mSwapchainImageReady       = nullptr;
        VkSemaphore mPrimaryCompletedSemaphore = nullptr;
        VkFence     mPrimaryCompletedFence     = nullptr;

        uint32_t mSwapchainImageIndex = 0;
    };
}  // namespace foray::base
