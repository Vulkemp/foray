#include "hsk_geo.hpp"

namespace hsk {

    VertexInputStateBuilder& VertexInputStateBuilder::AddVertexComponentBinding(VertexComponent component, uint32_t location)
    {
        if(location == UINT32_MAX)
        {
            location = NextLocation;
            NextLocation++;
        }
        Components.push_back(VertexComponentBinding{component, location});
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
                case VertexComponent::Uv0:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, Uv0)});
                    break;
                case VertexComponent::Uv1:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, Uv1)});
                    break;
                // case VertexComponent::Joint0:
                //     InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, Joint0)});
                //     break;
                // case VertexComponent::Weight0:
                //     InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, Weight0)});
                //     break;
                case VertexComponent::MaterialId:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32_SINT, offsetof(Vertex, MaterialId)});
                    break;
                case VertexComponent::MeshId:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32_SINT, offsetof(Vertex, MeshId)});
                    break;
                default:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{});
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