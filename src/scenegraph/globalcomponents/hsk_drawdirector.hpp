#pragma once
#include "../../memory/hsk_descriptorsethelper.hpp"
#include "../../memory/hsk_managedvectorbuffer.hpp"
#include "../../utility/hsk_framerotator.hpp"
#include "../hsk_component.hpp"

namespace hsk {

    struct DrawOp
    {
      public:
        uint64_t                   Order           = 0;
        Mesh*                      Target          = nullptr;
        std::vector<MeshInstance*> Instances       = {};
        uint32_t                   TransformOffset = 0;
    };

    class DrawDirector : public GlobalComponent, public Component::DrawCallback
    {
      public:
        inline DrawDirector() : mTransformBuffers()
        {
            for(uint32_t i = 0; i < INFLIGHT_FRAME_COUNT; i++)
            {
                mBufferInfosCurrent[i].resize(1);
                mBufferInfosPrevious[i].resize(1);
            }
        }

        void InitOrUpdate();

        void Draw(SceneDrawInfo&);

        /// @brief
        /// @param shaderStage - The shader stage in which camera ubo should be accessible. Defaults to vertex stage, where
        /// the camera matrix is usually used, but can also be set to be used in a raygen stage.
        /// @return
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MakeDescriptorInfosForCurrent(VkShaderStageFlags shaderStage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MakeDescriptorInfosForPrevious(VkShaderStageFlags shaderStage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);


      protected:
        /// @brief Per each inflight frame, have a storage buffer containing transforms
        FrameRotator<ManagedVectorBuffer<glm::mat4>, INFLIGHT_FRAME_COUNT> mTransformBuffers;
        /// @brief Buffer info vectors
        std::vector<VkDescriptorBufferInfo> mBufferInfosCurrent[INFLIGHT_FRAME_COUNT]  = {};
        std::vector<VkDescriptorBufferInfo> mBufferInfosPrevious[INFLIGHT_FRAME_COUNT] = {};
        /// @brief Draw Op structs store draw operation
        std::vector<DrawOp> mDrawOps    = {};
        bool                mFirstSetup = true;
        GeometryStore*      mGeo        = nullptr;
    };
}  // namespace hsk