#include "hsk_cameramanager.hpp"
#include "../components/hsk_camera.hpp"
#include "../hsk_scene.hpp"

namespace hsk {

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> CameraManager::MakeUboDescriptorInfos(VkShaderStageFlags shaderStage)
    {
        UpdateUboDescriptorBufferInfos();
        auto descriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, shaderStage);
        descriptorInfo->AddDescriptorSet(&mUboDescriptorBufferInfo);
        return descriptorInfo;
    }

    inline void CameraManager::UpdateUboDescriptorBufferInfos()
    {
        mUboDescriptorBufferInfo = {mUbo.GetUboBuffer().GetDeviceBuffer().GetVkDescriptorBufferInfo()};
    }

    void CameraManager::Update(const FrameUpdateInfo& updateInfo)
    {
        if(!!mSelectedCamera)
        {
            mSelectedCamera->UpdateUbo(mUbo.GetData());
            mUbo.UpdateTo(updateInfo.GetFrameNumber());
        }

        mUbo.CmdCopyToDevice(updateInfo.GetFrameNumber(), updateInfo.GetCommandBuffer());
    }

    CameraManager::CameraManager(const VkContext* context)
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

}  // namespace hsk