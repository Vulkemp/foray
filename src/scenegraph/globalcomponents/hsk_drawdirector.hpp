#pragma once
#include "../../memory/hsk_descriptorsethelper.hpp"
#include "../../memory/hsk_dualbuffer.hpp"
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

    class DrawDirector : public GlobalComponent, public Component::UpdateCallback, public Component::DrawCallback
    {
      public:
        inline DrawDirector()
        {
        }

        void InitOrUpdate();

        void CreateBuffers(size_t transformCount);
        void DestroyBuffers();

        virtual void Update(const FrameUpdateInfo&) override;
        virtual void Draw(SceneDrawInfo&) override;

        /// @brief
        /// @param shaderStage - The shader stage in which camera ubo should be accessible. Defaults to vertex stage, where
        /// the camera matrix is usually used, but can also be set to be used in a raygen stage.
        /// @return
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MakeDescriptorInfosForCurrent(VkShaderStageFlags shaderStage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MakeDescriptorInfosForPrevious(VkShaderStageFlags shaderStage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);


      protected:
        DualBuffer    mCurrentTransformBuffer;
        ManagedBuffer mPreviousTransformBuffer;

        std::vector<VkDescriptorBufferInfo> mCurrentDescriptorInfo;
        std::vector<VkDescriptorBufferInfo> mPreviousDescriptorInfo;

        /// @brief Draw Op structs store draw operation
        std::vector<DrawOp> mDrawOps    = {};
        bool                mFirstSetup = true;
        GeometryStore*      mGeo        = nullptr;
        uint32_t            mTotalCount = 0;
    };
}  // namespace hsk