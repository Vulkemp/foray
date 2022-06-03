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

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> Camera::GetUboDescriptorInfo()
    {
        size_t numUbos = 1;  // we load the complete ubo buffer as a single ubo buffer.

        auto descriptorInfo                = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->ShaderStageFlags   = VK_SHADER_STAGE_VERTEX_BIT;
        descriptorInfo->pImmutableSamplers = nullptr;
        descriptorInfo->DescriptorCount    = 1;
        descriptorInfo->DescriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        size_t numSets = 1;
        descriptorInfo->BufferInfos.resize(numSets);

        for(size_t setIndex = 0; setIndex < numSets; setIndex++)
        {
            descriptorInfo->BufferInfos[setIndex].resize(numUbos);
            for(size_t i = 0; i < numUbos; i++)
            {
                descriptorInfo->BufferInfos[setIndex][i] = mUbo.GetManagedBuffer().GetVkDescriptorBufferInfo();
            }
        }
        return descriptorInfo;
    }

    void Camera::InitDefaultViewMatrix()
    {
        glm::vec3 cameraEye    = glm::vec3(0.0f, 0.0f, 3.0f);
        glm::vec3 cameraCenter = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 cameraUp     = glm::vec3(0.0f, 1.0f, 0.0f);
        mUbo.GetUbo().ViewMat  = glm::lookAt(cameraEye, cameraCenter, cameraUp);
    }

    void Camera::InitDefaultProjectionMatrix() { mUbo.GetUbo().ProjectionMat = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f); }

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