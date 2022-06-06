#include "hsk_geo.hpp"
#include "../base/hsk_logger.hpp"

namespace hsk {

    NVertexInputStateBuilder& NVertexInputStateBuilder::AddVertexComponentBinding(EVertexComponent component, std::optional<uint32_t> location)
    {
        if(!location.has_value())
        {
            location = NextLocation;
            NextLocation++;
        }
        Components.push_back(NVertexComponentBinding{component, location.value()});
        return *this;
    }


    void NVertexInputStateBuilder::Build()
    {
        InputStateCI    = {};
        InputBindings   = {};
        InputAttributes = {};
        InputBindings.push_back(VkVertexInputBindingDescription{.binding = Binding, .stride = sizeof(NVertex), .inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX});

        for(auto& component : Components)
        {
            switch(component.Component)
            {
                case EVertexComponent::Position:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(NVertex, Pos)});
                    break;
                case EVertexComponent::Normal:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(NVertex, Normal)});
                    break;
                case EVertexComponent::Tangent:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(NVertex, Tangent)});
                    break;
                case EVertexComponent::Uv:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32G32_SFLOAT, offsetof(NVertex, Uv)});
                    break;
                case EVertexComponent::MaterialIndex:
                    InputAttributes.push_back(VkVertexInputAttributeDescription{component.Location, Binding, VK_FORMAT_R32_SINT, offsetof(NVertex, MaterialIndex)});
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