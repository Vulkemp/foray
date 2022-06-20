#include "hsk_camera.hpp"
#include "../../memory/hsk_descriptorsethelper.hpp"
#include "../../osi/hsk_event.hpp"
#include "../hsk_scene.hpp"

#undef near
#undef far

namespace hsk {
    Camera::Camera() : mUbo(true)
    {
        mUbo.GetManagedBuffer().SetName("CameraUbo");
    }

    void Camera::InitDefault()
    {
        SetViewMatrix();
        SetProjectionMatrix();
        mUbo.Init(GetScene()->GetContext(), true);
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> Camera::GetUboDescriptorInfo()
    {
        size_t numUbos = 1;  // we load the complete ubo buffer as a single ubo buffer.

        std::vector<VkDescriptorBufferInfo> bufferInfos({mUbo.GetManagedBuffer().GetVkDescriptorBufferInfo()});
        auto                                descriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, bufferInfos);
        return descriptorInfo;
    }

    void Camera::SetViewMatrix()
    {
        auto& ubo = mUbo.GetUbo();
        ubo.ViewMatrix = glm::lookAt(mEyePosition, mLookatPosition, mUpDirection);
        ubo.ProjectionViewMatrix = ubo.ProjectionMatrix * ubo.ViewMatrix;
    }

    void Camera::SetProjectionMatrix()
    {
        if(mVerticalFov == 0.f)
        {
            mVerticalFov = glm::radians(75.f);
        }
        if(mAspect == 0.f)
        {
            auto swapchainExtent = GetContext()->Swapchain.extent;
            mAspect              = CalculateAspect(swapchainExtent);
        }
        if(mNear == 0.f)
        {
            mNear = 0.1f;
        }
        if(mFar == 0.f)
        {
            mFar = 10000.f;
        }
        auto& ubo = mUbo.GetUbo();
        ubo.ProjectionMatrix = glm::perspective(mVerticalFov, mAspect, mNear, mFar);
        ubo.ProjectionViewMatrix = ubo.ProjectionMatrix * ubo.ViewMatrix;
    }

    void Camera::SetViewMatrix(const glm::vec3& eye, const glm::vec3& lookat, const glm::vec3& up)
    {
        mEyePosition    = eye;
        mLookatPosition = lookat;
        mUpDirection    = up;
        SetViewMatrix();
    }
    void Camera::SetProjectionMatrix(float verticalFov, float aspect, float near, float far)
    {
        mVerticalFov = verticalFov;
        mAspect      = aspect;
        mNear        = near;
        mFar         = far;
        SetProjectionMatrix();
    }

    void Camera::BeforeDraw(const FrameRenderInfo& renderInfo)
    {
        mUbo.Update();
    }
    void Camera::OnResized(VkExtent2D extent)
    {
        mAspect = CalculateAspect(extent);
        SetProjectionMatrix();
    }

    void Camera::Cleanup()
    {
        mUbo.Cleanup();
    }

}  // namespace hsk