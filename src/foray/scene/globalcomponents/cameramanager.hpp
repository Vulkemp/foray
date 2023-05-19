#pragma once
#include "../../util/managedubo.hpp"
#include "../camerauboblock.hpp"
#include "../component.hpp"
#include "../scene_declares.hpp"
#include <unordered_set>

namespace foray::scene::gcomp {

    /// @brief Manages the Camera matrix buffer and maintains a list of cameras in the scene
    class CameraManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        FORAY_GETTER_CR(Ubo)

        inline VkDescriptorBufferInfo GetVkDescriptorInfo() const { return mUbo.GetVkDescriptorBufferInfo(); }

        CameraManager(core::Context* context);
        virtual void RefreshCameraList();
        virtual void SelectCamera(ncomp::Camera* camera);
        virtual ~CameraManager();

        virtual void Update(SceneUpdateInfo& updateInfo) override;

        void GetCameras(std::vector<ncomp::Camera*>& cameras);

        FORAY_GETTER_V(SelectedCamera)

      protected:
        util::ManagedUbo<CameraUboBlock> mUbo;

        std::unordered_set<ncomp::Camera*> mCameras;

        ncomp::Camera* mSelectedCamera = nullptr;

        void UpdateUboDescriptorBufferInfos();
    };
}  // namespace foray::scene