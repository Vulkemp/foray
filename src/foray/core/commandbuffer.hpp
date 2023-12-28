#pragma once
#include "context.hpp"
#include "managedresource.hpp"
#include <vulkan/vulkan.h>

namespace foray::core {

    /// @brief VkCommandBuffer wrapper
    class CommandBuffer : public VulkanResource<vk::ObjectType::eCommandBuffer>
    {
      public:
        CommandBuffer(Context* context, VkCommandBufferLevel cmdBufferLvl = VK_COMMAND_BUFFER_LEVEL_PRIMARY, bool begin = false);
        virtual ~CommandBuffer();

        /// @brief vkBeginCommandBuffer();
        virtual void Begin();
        /// @brief vkEndCommandBuffer()
        virtual void End();
        /// @brief vkResetCommandBuffer()
        virtual void Reset(VkCommandBufferResetFlags flags = 0);

        virtual void SetName(std::string_view name) override;

        inline operator VkCommandBuffer() const { return mCommandBuffer; }

        FORAY_GETTER_V(CommandBuffer)
      protected:
        core::Context* mContext = nullptr;

        VkCommandBuffer mCommandBuffer{};
        bool            mIsRecording = false;
    };

    /// @brief Extension of the commandbuffer wrapper for temporary host synchronized command buffer execution
    class HostSyncCommandBuffer : public CommandBuffer
    {
      public:
        /// @brief Create based on the contexts device, command pool and queue.
        HostSyncCommandBuffer(Context* context, VkCommandBufferLevel cmdBufferLvl = VK_COMMAND_BUFFER_LEVEL_PRIMARY, bool begin = false);

        /// @brief Submits but doesn't synchronize. Use HasCompleted() and/or WaitForCompletion() to synchronize
        void Submit();
        /// @brief Submits and waits for the commandbuffer to be completed
        void SubmitAndWait();
        /// @brief Checks the fence for completion (non-blocking)
        bool HasCompleted();
        /// @brief Blocks CPU thread until commandbuffer has completed
        void WaitForCompletion();

        virtual ~HostSyncCommandBuffer();

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
        vk::PipelineStageFlags2 WaitStage = vk::PipelineStageFlagBits2::eAllCommands;

        /// @brief Shorthand for initializing a SemaphoreReference struct for a binary semaphore
        /// @param semaphore Semaphore
        /// @param waitStage Wait / Signal stage masks
        static SemaphoreReference Binary(VkSemaphore semaphore, vk::PipelineStageFlags2 waitStage = vk::PipelineStageFlagBits2::eAllCommands);
        /// @brief Shorthand for initializing a SemaphoreReference struct for a timeline semaphore
        /// @param semaphore Semaphore
        /// @param value Timeline value to wait for / signal
        /// @param waitStage Wait / Signal stage masks
        static SemaphoreReference Timeline(VkSemaphore semaphore, uint64_t value, vk::PipelineStageFlags2 waitStage = vk::PipelineStageFlagBits2::eAllCommands);

        operator VkSemaphoreSubmitInfo() const;
    };

    /// @brief Extension of the commandbuffer wrapper for device and/or host synchronized command buffer execution
    class DeviceSyncCommandBuffer : public CommandBuffer
    {
      public:
        inline DeviceSyncCommandBuffer(Context* context, VkCommandBufferLevel cmdBufferLvl = VK_COMMAND_BUFFER_LEVEL_PRIMARY, bool begin = false)
            : CommandBuffer(context, cmdBufferLvl, begin)
        {
        }

        /// @brief Adds a semaphore to wait for before execution of the commandbuffer
        virtual DeviceSyncCommandBuffer& AddWaitSemaphore(const SemaphoreReference& semaphore);
        /// @brief Adds a semaphore to signal after execution of commandbuffer has finished
        virtual DeviceSyncCommandBuffer& AddSignalSemaphore(const SemaphoreReference& semaphore);

        FORAY_PROPERTY_R(WaitSemaphores)
        FORAY_PROPERTY_R(SignalSemaphores)
        FORAY_PROPERTY_V(Fence)

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