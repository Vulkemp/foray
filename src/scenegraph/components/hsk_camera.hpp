#pragma once
#include "../../hsk_glm.hpp"
#include "../../memory/hsk_descriptorsethelper.hpp"
#include "../../memory/hsk_managedubo.hpp"
#include "../../utility/hsk_framerotator.hpp"
#include "../hsk_component.hpp"
#include <array>

namespace hsk {

    class Camera : public NodeComponent, public Component::OnResizedCallback
    {
      public:
        void InitDefault();

        inline glm::mat4& ProjectionMat() { return mProjectionMatrix; }
        inline glm::mat4& ViewMat() { return mViewMatrix; }

        void SetViewMatrix();
        void SetProjectionMatrix();

        void SetViewMatrix(const glm::vec3& eye, const glm::vec3& lookat, const glm::vec3& up);
        void SetProjectionMatrix(float verticalFov, float aspect, float near, float far);

        virtual void UpdateUbo(CameraUboBlock& uboblock);
        virtual void OnResized(VkExtent2D extent) override;

        inline static float CalculateAspect(const VkExtent2D extent);

        HSK_PROPERTY_GET(EyePosition)
        HSK_PROPERTY_CGET(EyePosition)
        HSK_PROPERTY_GET(LookatPosition)
        HSK_PROPERTY_CGET(LookatPosition)
        HSK_PROPERTY_GET(UpDirection)
        HSK_PROPERTY_CGET(UpDirection)

      protected:
        float     mVerticalFov      = 0;
        float     mAspect           = 0.f;
        float     mNear             = 0;
        float     mFar              = 0;
        glm::vec3 mEyePosition      = glm::vec3(0.f, 0.f, -1.f);
        glm::vec3 mLookatPosition   = glm::vec3(0.f, 0.f, 0.f);
        glm::vec3 mUpDirection      = glm::vec3(0.f, 1.f, 0.f);
        glm::mat4 mViewMatrix       = glm::mat4(1);
        glm::mat4 mProjectionMatrix = glm::mat4(1);
    };

    inline float Camera::CalculateAspect(const VkExtent2D extent)
    {
        return (float)extent.width / (float)extent.height;
    }
}  // namespace hsk