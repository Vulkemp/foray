#pragma once
#include "../base/foray_framerenderinfo.hpp"
#include "../foray_basics.hpp"
#include "../foray_glm.hpp"
#include "../foray_vulkan.hpp"
#include "../util/foray_hash.hpp"
#include "foray_scene_declares.hpp"

namespace foray::scene {
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

    struct SceneUpdateInfo
    {
      public:
        const base::FrameRenderInfo RenderInfo;
        VkCommandBuffer             CmdBuffer;

        inline SceneUpdateInfo(const base::FrameRenderInfo& renderInfo, base::CmdBufferIndex index);
        inline SceneUpdateInfo(const base::FrameRenderInfo& renderInfo, VkCommandBuffer cmdBuffer);
    };

    SceneUpdateInfo::SceneUpdateInfo(const base::FrameRenderInfo& renderInfo, base::CmdBufferIndex index) : RenderInfo(renderInfo), CmdBuffer(renderInfo.GetCommandBuffer(index)) {}
    SceneUpdateInfo::SceneUpdateInfo(const base::FrameRenderInfo& renderInfo, VkCommandBuffer cmdBuffer) : RenderInfo(renderInfo), CmdBuffer(cmdBuffer) {}

    struct SceneDrawInfo
    {
      public:
        const base::FrameRenderInfo RenderInfo;
        VkCommandBuffer             CmdBuffer;
        const VkPipelineLayout      PipelineLayout    = nullptr;
        DrawPushConstant            PushConstantState = {};

        inline SceneDrawInfo(const base::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout, base::CmdBufferIndex index);
        inline SceneDrawInfo(const base::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout, VkCommandBuffer cmdBuffer);

        inline void CmdPushConstant_TransformBufferOffset(uint32_t transformBufferOffset);
        inline void CmdPushConstant_MaterialIndex(int32_t materialIndex);
    };

    void SceneDrawInfo::CmdPushConstant_TransformBufferOffset(uint32_t transformBufferOffset)
    {
        PushConstantState.CmdPushConstant_TransformBufferOffset(CmdBuffer, PipelineLayout, transformBufferOffset);
    }

    void SceneDrawInfo::CmdPushConstant_MaterialIndex(int32_t materialIndex)
    {
        PushConstantState.CmdPushConstant_MaterialIndex(CmdBuffer, PipelineLayout, materialIndex);
    }

    SceneDrawInfo::SceneDrawInfo(const base::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout, base::CmdBufferIndex index)

        : RenderInfo(renderInfo), CmdBuffer(renderInfo.GetCommandBuffer(index)), PipelineLayout(pipelineLayout), PushConstantState()
    {
        CmdPushConstant_TransformBufferOffset(0);
        CmdPushConstant_MaterialIndex(-1);
    }

    SceneDrawInfo::SceneDrawInfo(const base::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout, VkCommandBuffer cmdBuffer)

        : RenderInfo(renderInfo), CmdBuffer(cmdBuffer), PipelineLayout(pipelineLayout), PushConstantState()
    {
        CmdPushConstant_TransformBufferOffset(0);
        CmdPushConstant_MaterialIndex(-1);
    }
}  // namespace foray::scene