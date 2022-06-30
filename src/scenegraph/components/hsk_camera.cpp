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
        for(size_t i = 0; i < 2; i++)
        {
            mUbos[i].SetName(fmt::format("Camera Ubo #{}", i));
        }
    }

    void Camera::InitDefault()
    {
        SetViewMatrix();
        SetProjectionMatrix();
        mUbos.Init(GetScene()->GetContext(), true);
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> Camera::GetUboDescriptorInfo(size_t index)
    {
        size_t numUbos = 1;  // we load the complete ubo buffer as a single ubo buffer.

        std::vector<VkDescriptorBufferInfo> bufferInfos({mUbos[index].GetManagedBuffer().GetVkDescriptorBufferInfo()});
        auto                                descriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, bufferInfos);
        return descriptorInfo;
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> Camera::GetUboDescriptorInfos()
    {
        size_t numUbos = 2;  // we load the complete ubo buffer as a single ubo buffer.

        std::vector<VkDescriptorBufferInfo> bufferInfos({mUbos[0].GetManagedBuffer().GetVkDescriptorBufferInfo(), mUbos[1].GetManagedBuffer().GetVkDescriptorBufferInfo()});
        auto                                descriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);
        descriptorInfo->AddDescriptorSet(std::vector<VkDescriptorBufferInfo>({mUbos[0].GetManagedBuffer().GetVkDescriptorBufferInfo()}));
        descriptorInfo->AddDescriptorSet(std::vector<VkDescriptorBufferInfo>({mUbos[1].GetManagedBuffer().GetVkDescriptorBufferInfo()}));
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
        ubo.Update();
    }
    void Camera::OnResized(VkExtent2D extent)
    {
        mAspect = CalculateAspect(extent);
        SetProjectionMatrix();
    }

    void Camera::Cleanup()
    {
        mUbos.Cleanup();
    }

}  // namespace hsk