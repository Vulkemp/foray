#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "../utility/hsk_deviceresource.hpp"
#include <vulkan/vulkan.h>

namespace hsk {

    class CommandBuffer : public DeviceResourceBase
    {
      public:
        CommandBuffer() = default;
        inline virtual ~CommandBuffer() { Cleanup(); }

        VkCommandBuffer Create(const VkContext* context, VkCommandBufferLevel cmdBufferLvl = VK_COMMAND_BUFFER_LEVEL_PRIMARY, bool begin = false);
        void            Begin();
        void            Submit(bool fireAndForget = false);
        void            FlushAndReset();
        void            WaitForCompletion();

        virtual void Cleanup() override;
        virtual bool Exists() const override { return mCommandBuffer; }

        inline operator VkCommandBuffer() { return mCommandBuffer; }
        inline operator const VkCommandBuffer() const { return mCommandBuffer; }

        HSK_PROPERTY_CGET(CommandBuffer)
      protected:
        const VkContext* mContext{};
        VkCommandBuffer  mCommandBuffer{};
        VkFence          mFence{};
    };

}  // namespace hsk