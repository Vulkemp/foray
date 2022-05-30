#include "hsk_camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace hsk {
    void Camera::InitFromTinyGltfCamera(const tinygltf::Camera& camera)
    {
        mUbo = new ManagedUbo<CameraUboBlock>(true);
        mUbo->Init(Context());
        ViewMat() = glm::identity<glm::mat4>();
        if(camera.type == "perspective")
        {
            InitFromTinyGltfCameraPerspective(camera.perspective);
        }
        else if(camera.type == "orthographic")
        {
            InitFromTinyGltfCameraOrthographic(camera.orthographic);
        }
    }

    void Camera::InitFromTinyGltfCameraPerspective(const tinygltf::PerspectiveCamera& camera) 
    {
        ProjectionMat() = glm::perspective(camera.yfov, camera.aspectRatio, camera.znear, camera.zfar);
    }
    void Camera::InitFromTinyGltfCameraOrthographic(const tinygltf::OrthographicCamera& camera) 
    {
        ProjectionMat() = glm::orthoLH(-camera.xmag, camera.xmag, -camera.ymag, camera.ymag, camera.znear, camera.zfar);
    }

    void Camera::Update()
    {
        mUbo->Update();
    }
    void Camera::Cleanup(){
        mUbo->Cleanup();
        if(mUbo != nullptr)
        {
            delete mUbo;
        }
    }

}  // namespace hsk