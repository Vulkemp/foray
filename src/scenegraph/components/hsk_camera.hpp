#pragma once
#include "../hsk_component.hpp"

namespace hsk {

    struct NCameraUboBlock
    {
        glm::mat4 ProjectionMatrix         = {};
        glm::mat4 ViewMatrix               = {};
        glm::mat4 PreviousProjectionMatrix = {};
        glm::mat4 PreviousViewMatrix       = {};
    };

    class NCamera : public NodeComponent, public Component::BeforeDrawCallback, public Component::OnEventCallback
    {
      public:
        NCamera();

        void InitDefault();

        void Update();

        void Cleanup();

        inline virtual ~NCamera() { Cleanup(); }

        HSK_PROPERTY_GET(Ubo)
        HSK_PROPERTY_CGET(Ubo)

        inline glm::mat4& ProjectionMat() { return mUbo.GetUbo().ProjectionMatrix; }
        inline glm::mat4& ViewMat() { return mUbo.GetUbo().ViewMatrix; }

        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetUboDescriptorInfo();

        void SetViewMatrix();
        void SetProjectionMatrix();

        void SetViewMatrix(const glm::vec3& eye, const glm::vec3& lookat, const glm::vec3& up);
        void SetProjectionMatrix(float verticalFov, float aspect, float near, float far);

        virtual void BeforeDraw(const FrameRenderInfo& renderInfo) override;
        virtual void OnEvent(std::shared_ptr<Event>& event) override;

        inline static float CalculateAspect(const VkExtent2D extent);
      protected:
        float mVerticalFov = 0;
        float mAspect = 0;
        float mNear = 0;
        float mFar = 0;
        glm::vec3 mEyePosition = glm::vec3(0.f, 0.f, -1.f);
        glm::vec3 mLookatPosition = glm::vec3(0.f, 0.f, 0.f);
        glm::vec3 mUpDirection = glm::vec3(0.f, 1.f, 0.f);
        ManagedUbo<NCameraUboBlock> mUbo;
    };

    inline float NCamera::CalculateAspect(const VkExtent2D extent){
        return (float)extent.width / (float)extent.height;
    }
}  // namespace hsk