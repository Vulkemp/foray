#pragma once
#include "../base/hsk_vkcontext.hpp"
#include <vulkan/vulkan.h>

namespace hsk {

    class SingleTimeCommandBuffer
    {
      public:
        SingleTimeCommandBuffer()          = default;
        virtual ~SingleTimeCommandBuffer() = default;

        VkCommandBuffer Create(const VkContext* context, VkCommandBufferLevel cmdBufferLvl = VK_COMMAND_BUFFER_LEVEL_PRIMARY, bool begin = false);
        void            Begin();
        void            Flush(bool free = false);

        HSK_PROPERTY_CGET(CommandBuffer)
      protected:
        const VkContext* mContext{};
        VkCommandBuffer  mCommandBuffer{};
    };

}  // namespace hsk