#include "foray_cameramanager.hpp"
#include "../components/foray_camera.hpp"
#include "../foray_scene.hpp"

namespace foray::scene::gcomp {

    void CameraManager::Update(SceneUpdateInfo& updateInfo)
    {
        if(!!mSelectedCamera)
        {
            mSelectedCamera->UpdateUbo(mUbo.GetData(), ncomp::Camera::CalculateAspect(updateInfo.RenderSize));
            mUbo.UpdateTo(updateInfo.RenderInfo.GetFrameNumber());
        }
        else
        {
            logger()->warn("CameraManager does not have a camera selected. No Image output will be generated. Camera list might have to be refreshed, or the scene does not contain a camera.");
        }

        mUbo.CmdCopyToDevice(updateInfo.RenderInfo.GetFrameNumber(), updateInfo.CmdBuffer);
    }

    CameraManager::CameraManager(core::Context* context) : Component::UpdateCallback(ORDER_DEVICEUPLOAD), mUbo(context, "Camera")
    {
    }
    void CameraManager::RefreshCameraList()
    {
        mCameras.clear();

        std::vector<Node*> nodesWithCamera;
        GetScene()->FindNodesWithComponent<ncomp::Camera>(nodesWithCamera);
        for(auto node : nodesWithCamera)
        {
            mCameras.emplace(node->GetComponent<ncomp::Camera>());
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
    void CameraManager::SelectCamera(ncomp::Camera* camera)
    {
        Assert(mCameras.contains(camera),
               "CameraManager::SelectCamera: Parameter 'camera' was not a tracked camera! Check parameter and consider running CameraManager::RefreshCameraList() before!");

        mSelectedCamera = camera;
    }
    void CameraManager::GetCameras(std::vector<ncomp::Camera*>& cameras)
    {
        for (ncomp::Camera* camera : mCameras)
        {
            cameras.push_back(camera);
        }
    }
    CameraManager::~CameraManager()
    {
        mSelectedCamera = nullptr;
        mCameras.clear();
    }

}  // namespace foray