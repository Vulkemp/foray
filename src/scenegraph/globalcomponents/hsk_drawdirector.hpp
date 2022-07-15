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

    struct alignas(16) TransformState
    {
        glm::mat4 CurrentWorldMatrix  = glm::mat4(1);
        glm::mat4 PreviousWorldMatrix = glm::mat4(1);
    };

    class DrawDirector : public GlobalComponent, public Component::DrawCallback
    {
      public:
        inline DrawDirector() : mTransformBuffers()
        {
            mBufferInfos[0].resize(1);
            mBufferInfos[1].resize(1);
        }

        void InitOrUpdate();

        void Draw(SceneDrawInfo&);

        /// @brief
        /// @param shaderStage - The shader stage in which camera ubo should be accessible. Defaults to vertex stage, where
        /// the camera matrix is usually used, but can also be set to be used in a raygen stage.
        /// @return
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MakeDescriptorInfos(VkShaderStageFlags shaderStage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);


      protected:
        /// @brief Per each inflight frame, have a storage buffer containing transforms
        FrameRotator<ManagedVectorBuffer<TransformState>, INFLIGHTFRAMECOUNT> mTransformBuffers;
        /// @brief Buffer info vectors
        std::vector<VkDescriptorBufferInfo> mBufferInfos[INFLIGHTFRAMECOUNT] = {};
        /// @brief Draw Op structs store draw operation
        std::vector<DrawOp> mDrawOps    = {};
        bool                mFirstSetup = true;
    };
}  // namespace hsk