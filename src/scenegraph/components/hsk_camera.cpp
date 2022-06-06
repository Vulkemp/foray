#include "hsk_camera.hpp"
#include "../../memory/hsk_descriptorsethelper.hpp"
#include "../../osi/hsk_event.hpp"
#include "../hsk_scene.hpp"

namespace hsk {
    NCamera::NCamera() : mUbo(true) { mUbo.GetManagedBuffer().SetName("CameraUbo"); }

    void NCamera::InitDefault()
    {
        SetViewMatrix();
        SetProjectionMatrix();
        mUbo.Init(GetScene()->GetContext(), true);
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> NCamera::GetUboDescriptorInfo()
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

    void NCamera::SetViewMatrix()
    {
        mUbo.GetUbo().ViewMatrix = glm::lookAt(mEyePosition, mLookatPosition, mUpDirection);
    }

    void NCamera::SetProjectionMatrix() 
    {
        if (mVerticalFov == 0.f){
            mVerticalFov = glm::radians(75.f);
        }
        if (mAspect == 0.f){
            auto swapchainExtent = GetContext()->Swapchain.extent;
            mAspect = CalculateAspect(swapchainExtent);
        }
        if (mNear == 0.f){
            mNear = 0.1f;
        }
        if (mFar == 0.f){
            mFar = 10000.f;
        }
         mUbo.GetUbo().ProjectionMatrix = glm::perspective(mVerticalFov, mAspect, mNear, mFar); }

    void NCamera::SetViewMatrix(const glm::vec3& eye, const glm::vec3& lookat, const glm::vec3& up) 
    {
        mEyePosition = eye;
        mLookatPosition = lookat;
        mUpDirection = up;
        SetViewMatrix();
    }
    void NCamera::SetProjectionMatrix(float verticalFov, float aspect, float near, float far) 
    {
        mVerticalFov = verticalFov;
        mAspect = aspect;
        mNear = near;
        mFar = far;
        SetProjectionMatrix();
    }

    void NCamera::BeforeDraw(const FrameRenderInfo& renderInfo) { mUbo.Update(); }
    void NCamera::OnEvent(std::shared_ptr<Event>& event) {
        auto windowResized = std::dynamic_pointer_cast<EventWindowResized>(event);
        if (windowResized){
            mAspect = (float)windowResized->Current().Width / (float)windowResized->Current().Height;
            SetProjectionMatrix();
        }
    }

    void NCamera::Cleanup() { mUbo.Cleanup(); }

}  // namespace hsk