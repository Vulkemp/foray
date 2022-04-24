#pragma once
#include "hsk_glTF_declares.hpp"
#include <glm/glm.hpp>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {
    struct Vertex
    {
        glm::vec3 Pos     = {};
        glm::vec3 Normal  = {};
        glm::vec2 Uv0     = {};
        glm::vec2 Uv1     = {};
        glm::vec4 Joint0  = {};
        glm::vec4 Weight0 = {};
    };

}  // namespace hsk