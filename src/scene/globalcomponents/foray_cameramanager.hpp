#pragma once
#include "../../core/foray_descriptorsethelper.hpp"
#include "../../util/foray_managedubo.hpp"
#include "../foray_camerauboblock.hpp"
#include "../foray_component.hpp"
#include "../foray_scene_declares.hpp"
#include <unordered_set>

namespace foray::scene {

    class CameraManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        /// @brief
        /// @param shaderStage - The shader stage in which camera ubo should be accessible. Defaults to vertex stage, where
        /// the camera matrix is usually used, but can also be set to be used in a raygen stage.
        /// @return
        std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> MakeUboDescriptorInfos(VkShaderStageFlags shaderStage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);

        FORAY_PROPERTY_ALLGET(Ubo)

        CameraManager(const core::VkContext* context);
        virtual void RefreshCameraList();
        virtual void SelectCamera(Camera* camera);
        virtual ~CameraManager();

        virtual void Update(const base::FrameUpdateInfo& updateInfo) override;


      protected:
        util::ManagedUbo<CameraUboBlock> mUbo;

        std::vector<VkDescriptorBufferInfo> mUboDescriptorBufferInfo;

        std::unordered_set<Camera*> mCameras;

        Camera* mSelectedCamera = nullptr;

        void UpdateUboDescriptorBufferInfos();
    };
}  // namespace foray