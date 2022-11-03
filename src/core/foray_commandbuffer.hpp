#pragma once
#include "foray_context.hpp"
#include "foray_managedresource.hpp"
#include <vulkan/vulkan.h>

namespace foray::core {

    /// @brief VkCommandBuffer wrapper
    class CommandBuffer : public VulkanResource<VkObjectType::VK_OBJECT_TYPE_COMMAND_BUFFER>
    {
      public:
        CommandBuffer() = default;
        inline virtual ~CommandBuffer() { Destroy(); }

        /// @brief Create based on the contexts device, command pool and queue.
        virtual VkCommandBuffer Create(Context* context, VkCommandBufferLevel cmdBufferLvl = VK_COMMAND_BUFFER_LEVEL_PRIMARY, bool begin = false);
        /// @brief vkBeginCommandBuffer();
        virtual void Begin();
        /// @brief vkEndCommandBuffer()
        virtual void End();
        /// @brief vkResetCommandBuffer()
        virtual void Reset(VkCommandBufferResetFlags flags = 0);

        /// @brief Destroys the associated resources
        virtual void Destroy() override;
        virtual bool Exists() const override { return mCommandBuffer; }
        virtual void SetName(std::string_view name) override;

        inline operator VkCommandBuffer() const { return mCommandBuffer; }

        FORAY_PROPERTY_CGET(CommandBuffer)
      protected:
        core::Context* mContext = nullptr;

        VkCommandBuffer mCommandBuffer{};
        bool            mIsRecording = false;
    };

    /// @brief Extension of the commandbuffer wrapper for temporary host synchronized command buffer execution
    class HostCommandBuffer : public CommandBuffer
    {
      public:
        /// @brief Create based on the contexts device, command pool and queue.
        virtual VkCommandBuffer Create(Context* context, VkCommandBufferLevel cmdBufferLvl = VK_COMMAND_BUFFER_LEVEL_PRIMARY, bool begin = false) override;

        /// @brief Submits but doesn't synchronize. Use HasCompleted() and/or WaitForCompletion() to synchronize
        void Submit();
        /// @brief Submits and waits for the commandbuffer to be completed
        void SubmitAndWait();
        /// @brief Checks the fence for completion (non-blocking)
        bool HasCompleted();
        /// @brief Blocks CPU thread until commandbuffer has completed
        void WaitForCompletion();

        virtual void Destroy() override;

        inline virtual ~HostCommandBuffer() { Destroy(); }

      protected:
        VkFence mFence = nullptr;
    };

    /// @brief Wraps a wait or signal semaphore action
    struct SemaphoreReference
    {
        /// @brief Type of semaphore
        VkSemaphoreType SemaphoreType = VkSemaphoreType::VK_SEMAPHORE_TYPE_BINARY;
        /// @brief Timeline value to signal/wait (ignored if binary semaphore)
        uint64_t TimelineValue = 0;
        /// @brief Semaphore
        VkSemaphore Semaphore = nullptr;
        /// @brief Stage masks
        VkPipelineStageFlags2 WaitStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        /// @brief Shorthand for initializing a SemaphoreReference struct for a binary semaphore
        /// @param semaphore Semaphore
        /// @param waitStage Wait / Signal stage masks
        static SemaphoreReference Binary(VkSemaphore semaphore, VkPipelineStageFlags2 waitStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
        /// @brief Shorthand for initializing a SemaphoreReference struct for a timeline semaphore
        /// @param semaphore Semaphore
        /// @param value Timeline value to wait for / signal
        /// @param waitStage Wait / Signal stage masks
        static SemaphoreReference Timeline(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 waitStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

        operator VkSemaphoreSubmitInfo() const;
    };

    /// @brief Extension of the commandbuffer wrapper for device and/or host synchronized command buffer execution
    class DeviceCommandBuffer : public CommandBuffer
    {
      public:
        /// @brief Adds a semaphore to wait for before execution of the commandbuffer
        virtual DeviceCommandBuffer& AddWaitSemaphore(const SemaphoreReference& semaphore);
        /// @brief Adds a semaphore to signal after execution of commandbuffer has finished
        virtual DeviceCommandBuffer& AddSignalSemaphore(const SemaphoreReference& semaphore);

        FORAY_PROPERTY_ALL(WaitSemaphores)
        FORAY_PROPERTY_ALL(SignalSemaphores)
        FORAY_PROPERTY_ALL(Fence)

        /// @brief Submits the commandbuffer
        virtual void Submit();

        /// @brief Appends a suitable submitinfo to the vector
        virtual void WriteToSubmitInfo(std::vector<VkSubmitInfo2>& submitInfos);

      protected:
        std::vector<SemaphoreReference> mWaitSemaphores;
        std::vector<SemaphoreReference> mSignalSemaphores;
        VkFence                         mFence = nullptr;
    };

}  // namespace foray::core