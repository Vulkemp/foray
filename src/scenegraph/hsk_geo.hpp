#pragma once
#include "hsk_scenegraph_declares.hpp"
#include "../hsk_glm.hpp"
#include <optional>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {

    enum class EVertexComponent
    {
        Position,
        Normal,
        Tangent,
        Uv
    };

    struct VertexComponentBinding
    {
        EVertexComponent Component;
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

        VertexInputStateBuilder& AddVertexComponentBinding(EVertexComponent component, std::optional<uint32_t> location = {});
        void                     Build();
    };

    struct Vertex
    {
        glm::vec3 Pos = {};
        glm::vec3 Normal = {};
        glm::vec3 Tangent = {};
        glm::vec2 Uv      = {};
    };

}  // namespace hsk