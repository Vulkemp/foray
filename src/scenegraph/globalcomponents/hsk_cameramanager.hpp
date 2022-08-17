#pragma once
#include "../../memory/hsk_descriptorsethelper.hpp"
#include "../../memory/hsk_managedubo.hpp"
#include "../hsk_camerauboblock.hpp"
#include "../hsk_component.hpp"
#include "../hsk_scenegraph_declares.hpp"
#include <unordered_set>

namespace hsk {

    class CameraManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        /// @brief
        /// @param shaderStage - The shader stage in which camera ubo should be accessible. Defaults to vertex stage, where
        /// the camera matrix is usually used, but can also be set to be used in a raygen stage.
        /// @return
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MakeUboDescriptorInfos(VkShaderStageFlags shaderStage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);

        HSK_PROPERTY_ALLGET(Ubo)

        CameraManager(const VkContext* context);
        virtual void RefreshCameraList();
        virtual void SelectCamera(Camera* camera);
        virtual ~CameraManager();

        virtual void Update(const FrameUpdateInfo& updateInfo) override;


      protected:
        ManagedUbo<CameraUboBlock> mUbo;

        std::vector<VkDescriptorBufferInfo> mUboDescriptorBufferInfo;

        std::unordered_set<Camera*> mCameras;

        Camera* mSelectedCamera = nullptr;

        void UpdateUboDescriptorBufferInfos();
    };
}  // namespace hsk