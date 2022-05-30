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

        void CreateDeviceLocalBuffer();
    };
}  // namespace hsk
