#include "hsk_camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace hsk {
    void Camera::InitFromTinyGltfCamera(const tinygltf::Camera& camera)
    {
        ViewMat() = glm::identity<glm::mat4>();
        if(camera.type == "perspective")
        {
            InitFromTinyGltfCameraPerspective(camera.perspective);
        }
        else if(camera.type == "orthographic")
        {
            InitFromTinyGltfCameraOrthographic(camera.orthographic);
        }
        mUbo.Init(Context(), true);
    }

    void Camera::InitDefault() {
        InitDefaultViewMatrix();
        InitDefaultProjectionMatrix();
        mUbo.Init(Context(), true);
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> Camera::GetUboDescriptorInfo()
    {
        auto descriptorInfo                = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        std::vector<VkDescriptorBufferInfo> bufferInfos    = {mUbo.GetManagedBuffer().GetVkDescriptorBufferInfo()};
        descriptorInfo->Init(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, bufferInfos);
        return descriptorInfo;
    }

    void Camera::InitDefaultViewMatrix()
    {
        glm::vec3 cameraEye    = glm::vec3(0.0f, 0.0f, 3.0f);
        glm::vec3 cameraCenter = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 cameraUp     = glm::vec3(0.0f, 1.0f, 0.0f);
        mUbo.GetUbo().ViewMatrix  = glm::lookAt(cameraEye, cameraCenter, cameraUp);
    }

    void Camera::InitDefaultProjectionMatrix() { mUbo.GetUbo().ProjectionMatrix = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 10000.0f); }

    void Camera::InitFromTinyGltfCameraPerspective(const tinygltf::PerspectiveCamera& camera)
    {
        ProjectionMat() = glm::perspective(camera.yfov, camera.aspectRatio, camera.znear, camera.zfar);
    }
    void Camera::InitFromTinyGltfCameraOrthographic(const tinygltf::OrthographicCamera& camera)
    {
        ProjectionMat() = glm::orthoLH(-camera.xmag, camera.xmag, -camera.ymag, camera.ymag, camera.znear, camera.zfar);
    }

    void Camera::Update() { mUbo.Update(); }
    void Camera::Cleanup() { mUbo.Cleanup(); }

}  // namespace hsk