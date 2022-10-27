#pragma once
#include "../../util/foray_managedubo.hpp"
#include "../foray_camerauboblock.hpp"
#include "../foray_component.hpp"
#include "../foray_scene_declares.hpp"
#include <unordered_set>

namespace foray::scene {

    class CameraManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        FORAY_PROPERTY_ALLGET(Ubo)

        inline VkDescriptorBufferInfo GetVkDescriptorInfo() const { return mUbo.GetVkDescriptorBufferInfo(); }

        CameraManager(core::Context* context);
        virtual void RefreshCameraList();
        virtual void SelectCamera(Camera* camera);
        virtual ~CameraManager();

        virtual void Update(SceneUpdateInfo& updateInfo) override;

        virtual int32_t GetOrder() const override { return ORDER_DEVICEUPLOAD; }

      protected:
        util::ManagedUbo<CameraUboBlock> mUbo;

        std::unordered_set<Camera*> mCameras;

        Camera* mSelectedCamera = nullptr;

        void UpdateUboDescriptorBufferInfos();
    };
}  // namespace foray::scene