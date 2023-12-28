#pragma once
#include "../glm.hpp"
#include "../vma.hpp"
#include "../vulkan.hpp"
#include "scene_declares.hpp"
#include <optional>
#include <vector>

namespace foray::scene {

    /// @brief Vertex Components
    enum class EVertexComponent
    {
        Position,
        Normal,
        Tangent,
        Uv
    };

    /// @brief Binding of component to shader input location
    struct VertexComponentBinding
    {
        EVertexComponent Component;
        uint32_t         Location;
    };

    /// @brief Helper for building a VkPipelineVertexInputStateCreateInfo struct
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

    /// @brief Vertex
    struct Vertex
    {
        glm::vec3 Pos     = {};
        glm::vec3 Normal  = {};
        glm::vec3 Tangent = {};
        glm::vec2 Uv      = {};
    };

}  // namespace foray::scene