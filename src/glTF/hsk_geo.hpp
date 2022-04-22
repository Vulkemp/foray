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

    // TODO: Redo these buffer wrappers

    struct VertexBuffer : public NoMoveDefaults
    {
        VkBuffer      Buffer     = VK_NULL_HANDLE;
        VmaAllocation Allocation = {};
    };

    struct IndexBuffer : public NoMoveDefaults
    {
        int           Count      = 0;  // TODO: Consider removing member
        VkBuffer      Buffer     = VK_NULL_HANDLE;
        VmaAllocation Allocation = {};
    };


}  // namespace hsk