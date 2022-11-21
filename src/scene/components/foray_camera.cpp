#include "foray_camera.hpp"
#include "../../osi/foray_event.hpp"
#include "../foray_camerauboblock.hpp"
#include "../foray_scene.hpp"
#include "foray_transform.hpp"
#include <spdlog/fmt/fmt.h>

#undef near
#undef far

namespace foray::scene::ncomp {
    void Camera::InitDefault()
    {
        SetViewMatrix();
        SetProjectionMatrix();
    }

    void Camera::SetViewMatrix()
    {
        Transform* transform = GetNode()->GetTransform();
        transform->RecalculateGlobalMatrix();
        mViewMatrix = glm::inverse(transform->GetGlobalMatrix());
    }

    void Camera::SetProjectionMatrix()
    {
        if(mVerticalFov == 0.f)
        {
            mVerticalFov = glm::radians(75.f);
        }
        if(mAspect == 0.f)
        {
            auto swapchainExtent = GetContext()->GetSwapchainSize();
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

    void Camera::SetProjectionMatrix(float verticalFov, float aspect, float near, float far)
    {
        mVerticalFov = verticalFov;
        mAspect      = aspect;
        mNear        = near;
        mFar         = far;
        SetProjectionMatrix();
    }

    void Camera::UpdateUbo(CameraUboBlock& uboblock)
    {
        uboblock.PreviousViewMatrix           = uboblock.ViewMatrix;
        uboblock.PreviousProjectionMatrix     = uboblock.ProjectionMatrix;
        uboblock.PreviousProjectionViewMatrix = uboblock.ProjectionViewMatrix;
        uboblock.ViewMatrix                   = mViewMatrix;
        uboblock.ProjectionMatrix             = mProjectionMatrix;
        uboblock.ProjectionViewMatrix         = mProjectionMatrix * mViewMatrix;
        uboblock.InverseViewMatrix            = glm::inverse(mViewMatrix);
        uboblock.InverseProjectionMatrix      = glm::inverse(mProjectionMatrix);
    }
    void Camera::OnResized(VkExtent2D extent)
    {
        mAspect = CalculateAspect(extent);
        SetProjectionMatrix();
    }
}  // namespace foray::scene::ncomp