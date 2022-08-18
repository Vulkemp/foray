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
        uint32_t TransformBufferOffset = 0;
        int32_t  MaterialIndex         = -1;

        inline static VkShaderStageFlags  GetShaderStageFlags();
        inline static VkPushConstantRange GetPushConstantRange();

        inline void CmdPushConstant_TransformBufferOffset(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t transformBufferOffset);
        inline void CmdPushConstant_MaterialIndex(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int32_t materialIndex);
    };

    inline VkShaderStageFlags DrawPushConstant::GetShaderStageFlags()
    {
        return VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    inline VkPushConstantRange DrawPushConstant::GetPushConstantRange()
    {
        return VkPushConstantRange{.stageFlags = GetShaderStageFlags(), .offset = 0, .size = sizeof(DrawPushConstant)};
    }

    inline void DrawPushConstant::CmdPushConstant_TransformBufferOffset(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t transformBufferOffset)
    {
        TransformBufferOffset = transformBufferOffset;
        vkCmdPushConstants(commandBuffer, pipelineLayout, DrawPushConstant::GetShaderStageFlags(), offsetof(DrawPushConstant, TransformBufferOffset), sizeof(TransformBufferOffset),
                           &TransformBufferOffset);
    }
    inline void DrawPushConstant::CmdPushConstant_MaterialIndex(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int32_t materialIndex)
    {
        MaterialIndex = materialIndex;
        vkCmdPushConstants(commandBuffer, pipelineLayout, DrawPushConstant::GetShaderStageFlags(), offsetof(DrawPushConstant, MaterialIndex), sizeof(MaterialIndex),
                           &MaterialIndex);
    }

    struct SceneDrawInfo
    {
      public:
        const hsk::FrameRenderInfo RenderInfo;
        const VkPipelineLayout     PipelineLayout    = nullptr;
        DrawPushConstant           PushConstantState = {};

        inline SceneDrawInfo(const hsk::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout);

        inline void CmdPushConstant_TransformBufferOffset(uint32_t transformBufferOffset);
        inline void CmdPushConstant_MaterialIndex(int32_t materialIndex);
    };

    void SceneDrawInfo::CmdPushConstant_TransformBufferOffset(uint32_t transformBufferOffset)
    {
        PushConstantState.CmdPushConstant_TransformBufferOffset(RenderInfo.GetCommandBuffer(), PipelineLayout, transformBufferOffset);
    }

    void SceneDrawInfo::CmdPushConstant_MaterialIndex(int32_t materialIndex)
    {
        PushConstantState.CmdPushConstant_MaterialIndex(RenderInfo.GetCommandBuffer(), PipelineLayout, materialIndex);
    }

    SceneDrawInfo::SceneDrawInfo(const hsk::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout)

        : RenderInfo(renderInfo), PipelineLayout(pipelineLayout), PushConstantState()
    {
        CmdPushConstant_TransformBufferOffset(0);
        CmdPushConstant_MaterialIndex(-1);
    }
}  // namespace hsk