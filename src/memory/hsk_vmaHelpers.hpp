#pragma once
#include "../hsk_basics.hpp"
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include "../base/hsk_vkcontext.hpp"

namespace hsk {

    // TODO: create all kind of helper functions to create simple buffers.
    void CreateSimpleBuffer();
    void CreateDeviceLocalBuffer();
}  // namespace hsk
