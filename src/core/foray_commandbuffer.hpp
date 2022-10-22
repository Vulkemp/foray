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
        virtual void End();
        virtual void Reset(VkCommandBufferResetFlags flags = 0);

        /// @brief Destroys the associated resources
        virtual void Destroy() override;
        virtual bool Exists() const override { return mCommandBuffer; }
        virtual void SetName(std::string_view name) override;

        inline operator VkCommandBuffer() { return mCommandBuffer; }
        inline operator const VkCommandBuffer() const { return mCommandBuffer; }

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

        void Submit();
        void SubmitAndWait();
        bool HasCompleted();
        void WaitForCompletion();

        virtual void Destroy() override;

        inline virtual ~HostCommandBuffer() { Destroy(); }

      protected:
        VkFence mFence = nullptr;
    };

    struct SemaphoreSubmit
    {
        VkSemaphoreType       SemaphoreType = VkSemaphoreType::VK_SEMAPHORE_TYPE_BINARY;
        uint64_t              TimelineValue = 0;
        VkSemaphore           Semaphore     = nullptr;
        VkPipelineStageFlags2 WaitStage     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        static SemaphoreSubmit Binary(VkSemaphore semaphore, VkPipelineStageFlags2 waitStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
        static SemaphoreSubmit Timeline(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 waitStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

        operator VkSemaphoreSubmitInfo() const;
    };

    /// @brief Extension of the commandbuffer wrapper for device and/or host synchronized command buffer execution
    class DeviceCommandBuffer : public CommandBuffer
    {
      public:
        virtual DeviceCommandBuffer& AddWaitSemaphore(const SemaphoreSubmit& semaphore);
        virtual DeviceCommandBuffer& AddSignalSemaphore(const SemaphoreSubmit& semaphore);

        FORAY_PROPERTY_ALL(WaitSemaphores)
        FORAY_PROPERTY_ALL(SignalSemaphores)
        FORAY_PROPERTY_ALL(Fence)

        virtual void Submit();

        virtual void ExternalSubmit(std::vector<VkSubmitInfo2>& submitInfos);

      protected:
        std::vector<SemaphoreSubmit> mWaitSemaphores;
        std::vector<SemaphoreSubmit> mSignalSemaphores;
        VkFence                      mFence = nullptr;
    };

}  // namespace foray::core