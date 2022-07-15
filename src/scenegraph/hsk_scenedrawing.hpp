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

        inline static VkShaderStageFlags  GetShaderStageFlags();
        inline static VkPushConstantRange GetPushConstantRange();

        inline void CmdPushConstant_TransformBufferOffset(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t transformBufferOffset);
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

    struct SceneDrawInfo
    {
      public:
        const hsk::FrameRenderInfo RenderInfo;
        const VkPipelineLayout     PipelineLayout           = nullptr;
        DrawPushConstant           PushConstantState        = {};
        GeometryBufferSet*         CurrentlyBoundGeoBuffers = nullptr;

        inline SceneDrawInfo(const hsk::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout);

        inline void CmdPushConstant(uint32_t transformBufferOffset);
    };

    void SceneDrawInfo::CmdPushConstant(uint32_t transformBufferOffset)
    {
        PushConstantState.CmdPushConstant_TransformBufferOffset(RenderInfo.GetCommandBuffer(), PipelineLayout, transformBufferOffset);
    }

    SceneDrawInfo::SceneDrawInfo(const hsk::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout)

        : RenderInfo(renderInfo), PipelineLayout(pipelineLayout), PushConstantState()
    {
        CmdPushConstant(0);
    }
}  // namespace hsk