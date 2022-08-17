#include "hsk_camera.hpp"
#include "../../memory/hsk_descriptorsethelper.hpp"
#include "../../osi/hsk_event.hpp"
#include "../hsk_scene.hpp"
#include <spdlog/fmt/fmt.h>

#undef near
#undef far

namespace hsk {
    Camera::Camera() : mUbos(true)
    {
        for(size_t i = 0; i < INFLIGHT_FRAME_COUNT; i++)
        {
            mUboDescriptorBufferInfosSets[i].resize(1);
            mUbos[i].SetName(fmt::format("Camera Ubo #{}", i));
        }
    }

    void Camera::InitDefault()
    {
        SetViewMatrix();
        SetProjectionMatrix();
        mUbos.Init(GetScene()->GetContext(), true);
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> Camera::MakeUboDescriptorInfos(VkShaderStageFlags shaderStage)
    {
        UpdateUboDescriptorBufferInfos();
        auto descriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, shaderStage);
        for(size_t i = 0; i < INFLIGHT_FRAME_COUNT; i++)
        {
            descriptorInfo->AddDescriptorSet(&mUboDescriptorBufferInfosSets[i]);
        }
        return descriptorInfo;
    }

    void Camera::SetViewMatrix()
    {
        mViewMatrix = glm::lookAt(mEyePosition, mLookatPosition, mUpDirection);
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
        mProjectionMatrix = glm::perspective(mVerticalFov, mAspect, mNear, mFar);
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
        auto& ubo                            = mUbos[renderInfo.GetFrameNumber()];
        auto& uboData                        = ubo.GetUbo();
        uboData.PreviousViewMatrix           = uboData.ViewMatrix;
        uboData.PreviousProjectionMatrix     = uboData.ProjectionMatrix;
        uboData.PreviousProjectionViewMatrix = uboData.ProjectionViewMatrix;
        uboData.ViewMatrix                   = mViewMatrix;
        uboData.ProjectionMatrix             = mProjectionMatrix;
        uboData.ProjectionViewMatrix         = mProjectionMatrix * mViewMatrix;
        uboData.InverseViewMatrix            = glm::inverse(mViewMatrix);
        uboData.InverseProjectionMatrix      = glm::inverse(mProjectionMatrix);
        ubo.Update();
    }
    void Camera::OnResized(VkExtent2D extent)
    {
        mAspect = CalculateAspect(extent);
        SetProjectionMatrix();
    }

    void Camera::Destroy()
    {
        mUbos.Destroy();
    }

}  // namespace hsk