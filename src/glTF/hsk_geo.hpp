#pragma once
#include "hsk_glTF_declares.hpp"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace hsk {
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv0;
        glm::vec2 uv1;
        glm::vec4 joint0;
        glm::vec4 weight0;
    };

    struct Vertices
    {
        VkBuffer      buffer = VK_NULL_HANDLE;
        VmaAllocation allocation;
    };

    struct Indices
    {
        int           count;
        VkBuffer      buffer = VK_NULL_HANDLE;
        VmaAllocation allocation;
    };


}  // namespace hsk