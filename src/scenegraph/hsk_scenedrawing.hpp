#pragma once
#include "hsk_scenegraph_declares.hpp"
#include "../base/hsk_framerenderinfo.hpp"
#include "../hsk_basics.hpp"
#include <vulkan/vulkan.h>

namespace hsk {
    struct DrawPushConstant
    {
      public:
        int32_t MeshInstanceIndex = -1;

        inline static VkShaderStageFlags  GetShaderStageFlags();
        inline static VkPushConstantRange GetPushConstantRange();
    };

    inline VkShaderStageFlags DrawPushConstant::GetShaderStageFlags()
    {
        return VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    inline VkPushConstantRange DrawPushConstant::GetPushConstantRange()
    {
        return VkPushConstantRange{.stageFlags = GetShaderStageFlags(), .offset = 0, .size = sizeof(DrawPushConstant)};
    }

    struct SceneDrawInfo
    {
      public:
        const hsk::FrameRenderInfo RenderInfo;
        const VkPipelineLayout     PipelineLayout           = nullptr;
        DrawPushConstant           PushConstantState        = {};
        GeometryBufferSet*         CurrentlyBoundGeoBuffers = nullptr;

        inline SceneDrawInfo(const hsk::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout);

        inline void CmdPushConstant(int32_t meshInstanceIndex);
    };

    void SceneDrawInfo::CmdPushConstant(int32_t meshInstanceIndex)
    {
        PushConstantState = DrawPushConstant{.MeshInstanceIndex = meshInstanceIndex};
        vkCmdPushConstants(RenderInfo.GetCommandBuffer(), PipelineLayout, DrawPushConstant::GetShaderStageFlags(), 0, sizeof(DrawPushConstant), &PushConstantState);
    }

    SceneDrawInfo::SceneDrawInfo(const hsk::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout)

        : RenderInfo(renderInfo), PipelineLayout(pipelineLayout), PushConstantState(DrawPushConstant{.MeshInstanceIndex = -1})
    {
        CmdPushConstant(-1);
    }
}  // namespace hsk