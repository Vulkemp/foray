#include "foray_cameramanager.hpp"
#include "../components/foray_camera.hpp"
#include "../foray_scene.hpp"

namespace foray::scene {

    std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> CameraManager::MakeUboDescriptorInfos(VkShaderStageFlags shaderStage)
    {
        UpdateUboDescriptorBufferInfos();
        auto descriptorInfo = std::make_shared<core::DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, shaderStage);
        descriptorInfo->AddDescriptorSet(&mUboDescriptorBufferInfo);
        return descriptorInfo;
    }

    inline void CameraManager::UpdateUboDescriptorBufferInfos()
    {
        mUboDescriptorBufferInfo = {mUbo.GetUboBuffer().GetDeviceBuffer().GetVkDescriptorBufferInfo()};
    }

    void CameraManager::Update(SceneUpdateInfo& updateInfo)
    {
        if(!!mSelectedCamera)
        {
            mSelectedCamera->UpdateUbo(mUbo.GetData());
            mUbo.UpdateTo(updateInfo.RenderInfo.GetFrameNumber());
        }

        mUbo.CmdCopyToDevice(updateInfo.RenderInfo.GetFrameNumber(), updateInfo.CmdBuffer);
    }

    CameraManager::CameraManager(core::Context* context)
    {
        mUbo.Create(context, "Camera");
    }
    void CameraManager::RefreshCameraList()
    {
        mCameras.clear();

        std::vector<Node*> nodesWithCamera;
        GetScene()->FindNodesWithComponent<Camera>(nodesWithCamera);
        for(auto node : nodesWithCamera)
        {
            mCameras.emplace(node->GetComponent<Camera>());
        }

        if(!mSelectedCamera || !mCameras.contains(mSelectedCamera))
        {
            if(mCameras.size() > 0)
            {
                mSelectedCamera = *(mCameras.begin());
            }
            else
            {
                mSelectedCamera = nullptr;
            }
        }
    }
    void CameraManager::SelectCamera(Camera* camera)
    {
        Assert(mCameras.contains(camera),
               "CameraManager::SelectCamera: Parameter 'camera' was not a tracked camera! Check parameter and consider running CameraManager::RefreshCameraList() before!");

        mSelectedCamera = camera;
    }
    CameraManager::~CameraManager()
    {
        mSelectedCamera = nullptr;
        mCameras.clear();
        mUboDescriptorBufferInfo.clear();
        mUbo.Destroy();
    }

}  // namespace foray