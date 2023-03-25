#include "foray_camera.hpp"
#include "../foray_camerauboblock.hpp"
#include "../foray_scene.hpp"
#include "foray_transform.hpp"
#include <spdlog/fmt/fmt.h>

#undef near
#undef far

namespace foray::scene::ncomp {
    void Camera::InitDefault()
    {
        SetProjection();
    }

    void Camera::SetProjection()
    {
        if(mVerticalFov == 0.f)
        {
            mVerticalFov = glm::radians(75.f);
        }
        if(mNear == 0.f)
        {
            mNear = 0.1f;
        }
        if(mFar == 0.f)
        {
            mFar = 10000.f;
        }
    }

    void Camera::SetProjection(float verticalFov, float near, float far)
    {
        mVerticalFov = verticalFov;
        mNear        = near;
        mFar         = far;
        SetProjection();
    }

    void Camera::UpdateUbo(CameraUboBlock& uboblock, float aspect)
    {
        Transform* transform                  = GetNode()->GetTransform();
        glm::mat4  viewMat                    = glm::inverse(transform->GetGlobalMatrix());
        uboblock.PreviousViewMatrix           = uboblock.ViewMatrix;
        uboblock.PreviousProjectionMatrix     = uboblock.ProjectionMatrix;
        uboblock.PreviousProjectionViewMatrix = uboblock.ProjectionViewMatrix;
        uboblock.ViewMatrix                   = viewMat;
        uboblock.ProjectionMatrix             = glm::perspective(mVerticalFov, aspect, mNear, mFar);
        uboblock.ProjectionViewMatrix         = uboblock.ProjectionMatrix * viewMat;
        uboblock.InverseViewMatrix            = transform->GetGlobalMatrix();
        uboblock.InverseProjectionMatrix      = glm::inverse(uboblock.ProjectionMatrix);
    }
}  // namespace foray::scene::ncomp