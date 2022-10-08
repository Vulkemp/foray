#pragma once
#include "foray_deviceresource.hpp"
#include "foray_vkcontext.hpp"
#include <vulkan/vulkan.h>

namespace foray::core {

    /// @brief VkCommandBuffer wrapper
    class CommandBuffer : public DeviceResourceBase
    {
      public:
        CommandBuffer() = default;
        inline virtual ~CommandBuffer() { Destroy(); }

        /// @brief Create based on the contexts device, command pool and queue.
        VkCommandBuffer Create(const VkContext* context, VkCommandBufferLevel cmdBufferLvl = VK_COMMAND_BUFFER_LEVEL_PRIMARY, bool begin = false);
        /// @brief Create based on custom vulkan context
        VkCommandBuffer Create(VkDevice device, VkCommandPool cmdPool, Queue queue, VkCommandBufferLevel cmdBufferLvl = VK_COMMAND_BUFFER_LEVEL_PRIMARY, bool begin = false);
        /// @brief vkBeginCommandBuffer();
        void Begin();
        /// @brief vkSubmitCommandBuffer();
        /// @remark If fireAndForget is not set, will await signal to the included fence to assure the command buffer has run to completion
        void Submit(bool fireAndForget = false);
        /// @brief Submit, await completion, then reset
        void FlushAndReset();
        /// @brief Waits for the included fence to signal the command buffer has finished executing
        void WaitForCompletion();

        /// @brief Destroys the associated resources
        virtual void Destroy() override;
        virtual bool Exists() const override { return mCommandBuffer; }

        inline operator VkCommandBuffer() { return mCommandBuffer; }
        inline operator const VkCommandBuffer() const { return mCommandBuffer; }

        FORAY_PROPERTY_CGET(CommandBuffer)
      protected:
        struct Context
        {
            VkDevice      Device;
            VkCommandPool Pool;
            Queue         ExecutingQueue;
        } mContext = {};

        VkCommandBuffer mCommandBuffer{};
        VkFence         mFence{};
    };

}  // namespace foray::core