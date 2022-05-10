#pragma once
#include "hsk_glTF_declares.hpp"
#include <glm/glm.hpp>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {

    enum class VertexComponent
    {
        Position,
        Normal,
        Tangent,
        Uv0,
        Uv1,
        // Joint0,
        // Weight0,
        MaterialId,
        MeshId
    };

    struct VertexComponentBinding
    {
        VertexComponent Component;
        uint32_t        Location;
    };

    struct VertexInputStateBuilder
    {
        std::vector<VertexComponentBinding> Components;
        uint32_t                            Binding      = 0;
        uint32_t                            NextLocation = 0;

        std::vector<VkVertexInputAttributeDescription> InputAttributes{};
        std::vector<VkVertexInputBindingDescription>   InputBindings{};
        VkPipelineVertexInputStateCreateInfo           InputStateCI{};

        VertexInputStateBuilder& AddVertexComponentBinding(VertexComponent component, uint32_t location = UINT32_MAX);
        void Build();
    };

    struct Vertex
    {
        glm::vec3 Pos     = {};
        glm::vec3 Normal  = {};
        glm::vec3 Tangent = {};
        glm::vec2 Uv0     = {};
        glm::vec2 Uv1     = {};
        // glm::vec4 Joint0     = {};
        // glm::vec4 Weight0    = {};
        uint32_t MaterialId = {};
        uint32_t MeshId     = {};
    };

}  // namespace hsk