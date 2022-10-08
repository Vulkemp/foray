#pragma once
#include "../foray_glm.hpp"
#include "../foray_vma.hpp"
#include "../foray_vulkan.hpp"
#include "foray_scene_declares.hpp"
#include <optional>
#include <vector>

namespace foray::scene {

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
        uint32_t         Location;
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
        glm::vec3 Pos     = {};
        glm::vec3 Normal  = {};
        glm::vec3 Tangent = {};
        glm::vec2 Uv      = {};
    };

}  // namespace foray::scene