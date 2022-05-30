#pragma once
#include "../hsk_basics.hpp"
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include "../base/hsk_vkcontext.hpp"

namespace hsk {

    class ManagedBuffer;
    // TODO: create all kind of helper functions to create simple buffers.

    /// @brief Static class providing various helper functions regarding vulkan buffers and images.
    class VmaHelpers
    {
      public:
        /// @brief Creates a buffer that that can be mapped and serves as a memory transfer source
        /// @param outManagedBuffer - 
        /// @param context - The vk context.
        static void CreateStagingBuffer(ManagedBuffer* outManagedBuffer, const VkContext* context, void* data, size_t size);

        void CreateDeviceLocalBuffer();
    };
}  // namespace hsk
