#pragma once
#include "../hsk_basics.hpp"
#include <vulkan/vulkan.h>

namespace hsk {
    struct SceneDrawInfo
    {
        VkCommandBuffer  CmdBuffer      = {};
        VkPipelineLayout PipelineLayout = {};
    };
}  // namespace hsk
