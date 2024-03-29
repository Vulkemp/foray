#pragma once
#include "../../foray_glm.hpp"
#include "../../util/foray_managedubo.hpp"
#include "../foray_component.hpp"
#include <array>

namespace foray::scene::ncomp {
    /// @brief Defines a camera with projection matrix
    class Camera : public NodeComponent, public Component::OnResizedCallback
    {
      public:
        void InitDefault();

        inline glm::mat4& ProjectionMat() { return mProjectionMatrix; }

        void SetProjectionMatrix();

        void SetProjectionMatrix(float verticalFov, float aspect, float near, float far);

        virtual void UpdateUbo(CameraUboBlock& uboblock);
        virtual void OnResized(VkExtent2D extent) override;

        inline static float CalculateAspect(const VkExtent2D extent);

        FORAY_PROPERTY_R(UpDirection)
        FORAY_PROPERTY_V(VerticalFov)
        FORAY_PROPERTY_V(Aspect)
        FORAY_PROPERTY_V(Near)
        FORAY_PROPERTY_V(Far)

      protected:
        float     mVerticalFov      = 0;
        float     mAspect           = 0.f;
        float     mNear             = 0;
        float     mFar              = 0;
        glm::vec3 mUpDirection      = glm::vec3(0.f, 1.f, 0.f);
        glm::mat4 mProjectionMatrix = glm::mat4(1);
    };

    inline float Camera::CalculateAspect(const VkExtent2D extent)
    {
        return (float)extent.width / (float)extent.height;
    }
}  // namespace foray