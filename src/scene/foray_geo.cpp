#include "foray_geo.hpp"
#include "../foray_logger.hpp"

namespace foray::scene {

    VertexInputStateBuilder& VertexInputStateBuilder::AddVertexComponentBinding(EVertexComponent component, std::optional<uint32_t> location)
    {
        if(!location.has_value())
        {
            location = NextLocation;
            NextLocation++;
        }
        Components.push_back(VertexComponentBinding{component, location.value()});
        return *this;
    }


    void VertexInputStateBuilder::Build()
    {
        Stride          = Stride > 0 ? Stride : (uint32_t)sizeof(Vertex);
        InputStateCI    = {};
        InputBindings   = {};
        InputAttributes = {};
        InputBindings.push_back(VkVertexInputBindingDescription{.binding = Binding, .stride = Stride, .inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX});

        for(auto& component : Components)
        {
            switch(component.Component)
            {
                case EVertexComponent::Position:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Pos)});
                    break;
                case EVertexComponent::Normal:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Normal)});
                    break;
                case EVertexComponent::Tangent:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Tangent)});
                    break;
                case EVertexComponent::Uv:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, Uv)});
                    break;
                default:
                    Exception::Throw("Failed to add vertex component. This component type has no switch case defined!");
                    break;
            }
        }

        InputStateCI.sType                           = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        InputStateCI.vertexBindingDescriptionCount   = InputBindings.size();
        InputStateCI.pVertexBindingDescriptions      = InputBindings.data();
        InputStateCI.vertexAttributeDescriptionCount = InputAttributes.size();
        InputStateCI.pVertexAttributeDescriptions    = InputAttributes.data();
    }
}  // namespace foray