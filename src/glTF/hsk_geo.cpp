#include "hsk_geo.hpp"
#include "../base/hsk_logger.hpp"

namespace hsk {

    VertexInputStateBuilder& VertexInputStateBuilder::AddVertexComponentBinding(VertexComponent component, std::optional<uint32_t> location)
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
        InputStateCI    = {};
        InputBindings   = {};
        InputAttributes = {};
        InputBindings.push_back(VkVertexInputBindingDescription({Binding, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}));

        for(auto& component : Components)
        {
            switch(component.Component)
            {
                case VertexComponent::Position:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Pos)});
                    break;
                case VertexComponent::Normal:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Normal)});
                    break;
                case VertexComponent::Tangent:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Tangent)});
                    break;
                case VertexComponent::Uv:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, Uv)});
                    break;
                case VertexComponent::MaterialIndex:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32_SINT, offsetof(Vertex, MaterialIndex)});
                    break;
                case VertexComponent::MeshId:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32_SINT, offsetof(Vertex, MeshId)});
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
}  // namespace hsk