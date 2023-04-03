#pragma once
#include "../foray_glm.hpp"
#include "../foray_vma.hpp"
#include "../foray_vulkan.hpp"
#include "foray_scene_declares.hpp"
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
        uint32_t                            Stride       = 0;

        std::vector<VkVertexInputAttributeDescription> InputAttributes{};
        std::vector<VkVertexInputBindingDescription>   InputBindings{};
        VkPipelineVertexInputStateCreateInfo           InputStateCI{};

        VertexInputStateBuilder& AddVertexComponentBinding(EVertexComponent component, std::optional<uint32_t> location = {});
        VertexInputStateBuilder& SetStride(uint32_t stride) { Stride = stride; return *this; }
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