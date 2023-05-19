#pragma once
#include "../../glm.hpp"
#include "../../util/managedubo.hpp"
#include "../component.hpp"
#include <array>

namespace foray::scene::ncomp {
    /// @brief Defines a camera with projection matrix
    class Camera : public NodeComponent
    {
      public:
        void InitDefault();

        void SetProjection();

        void SetProjection(float verticalFov, float near, float far);

        virtual void UpdateUbo(CameraUboBlock& uboblock, float aspect);

        inline static float CalculateAspect(const VkExtent2D extent);

        FORAY_PROPERTY_R(UpDirection)
        FORAY_PROPERTY_V(VerticalFov)
        FORAY_PROPERTY_V(Near)
        FORAY_PROPERTY_V(Far)

      protected:
        float     mVerticalFov      = 0;
        float     mNear             = 0;
        float     mFar              = 0;
        glm::vec3 mUpDirection      = glm::vec3(0.f, 1.f, 0.f);
    };

    inline float Camera::CalculateAspect(const VkExtent2D extent)
    {
        return (float)extent.width / (float)extent.height;
    }
}  // namespace foray