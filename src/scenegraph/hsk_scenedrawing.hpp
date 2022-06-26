#pragma once
#include "../base/hsk_framerenderinfo.hpp"
#include "../hsk_basics.hpp"
#include "../hsk_glm.hpp"
#include "../utility/hsk_hash.hpp"
#include "hsk_scenegraph_declares.hpp"
#include <vulkan/vulkan.h>

namespace hsk {
    struct DrawPushConstant
    {
      public:
        glm::mat4 ModelWorldMatrix         = glm::mat4(1);
        glm::mat4 PreviousModelWorldMatrix = glm::mat4(1);
        int32_t   MeshInstanceIndex        = -1;

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

        inline void CmdPushConstant(int32_t meshInstanceIndex, const glm::mat4& worldMatrix, const glm::mat4& prevWorldMatrix);
    };

    void SceneDrawInfo::CmdPushConstant(int32_t meshInstanceIndex, const glm::mat4& worldMatrix, const glm::mat4& prevWorldMatrix)
    {
        PushConstantState = DrawPushConstant{.ModelWorldMatrix = worldMatrix, .PreviousModelWorldMatrix = prevWorldMatrix, .MeshInstanceIndex = meshInstanceIndex};
        vkCmdPushConstants(RenderInfo.GetCommandBuffer(), PipelineLayout, DrawPushConstant::GetShaderStageFlags(), 0, sizeof(DrawPushConstant), &PushConstantState);
    }

    SceneDrawInfo::SceneDrawInfo(const hsk::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout)

        : RenderInfo(renderInfo), PipelineLayout(pipelineLayout), PushConstantState()
    {
        CmdPushConstant(-1, glm::mat4(1), glm::mat4(1));
    }
}  // namespace hsk