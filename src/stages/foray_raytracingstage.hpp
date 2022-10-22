#pragma once
#include "../core/foray_context.hpp"
#include "../core/foray_managedbuffer.hpp"
#include "../core/foray_managedimage.hpp"
#include "../core/foray_shadermodule.hpp"
#include "../rtpipe/foray_rtpipeline.hpp"
#include "../scene/foray_scene.hpp"
#include "foray_rasterizedRenderStage.hpp"

// heavily inspired by https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/raytracing_basic/raytracing_basic.cpp
namespace foray::stages {

    class RaytracingStage : public RenderStage
    {
      public:
        RaytracingStage() = default;

        virtual void Init();
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        inline static constexpr std::string_view RaytracingRenderTargetName = "RaytraycingRenderTarget";

        virtual void OnResized(const VkExtent2D& extent) override;

      protected:
        scene::Scene*             mScene;
        std::vector<VkClearValue> mClearValues;

        virtual void CreateFixedSizeComponents() override;
        virtual void DestroyFixedComponents() override;
        virtual void CreateResolutionDependentComponents() override;
        virtual void DestroyResolutionDependentComponents() override;

        virtual void PrepareAttachments();
        virtual void SetupDescriptors();
        virtual void CreateDescriptorSets();
        virtual void UpdateDescriptors();
        virtual void CreatePipelineLayout();
        virtual void CreateRaytraycingPipeline();

        virtual void ReloadShaders();
        virtual void DestroyShaders() {}

        std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> GetAccelerationStructureDescriptorInfo(bool rebuild = false);
        std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> GetRenderTargetDescriptorInfo(bool rebuild = false);

        /// @brief Storage image that the ray generation shader will be writing to.
        core::ManagedImage mRaytracingRenderTarget;

        struct SampledImage
        {
            bool                                                       IsSet   = false;
            core::ManagedImage*                                        Image   = nullptr;
            VkSampler                                                  Sampler = nullptr;
            std::vector<VkDescriptorImageInfo>                         DescriptorImageInfos;
            std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> DescriptorInfo{};

            inline SampledImage() {}

            void                                                       Create(core::Context* context, core::ManagedImage* image, bool initateSampler = true);
            void                                                       Destroy(core::Context* context);
            void                                                       UpdateDescriptorInfos();
            std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> GetDescriptorInfo(bool rebuild = false);
        } mEnvMap, mNoiseSource;

        inline static constexpr VkShaderStageFlags RTSTAGEFLAGS =
            VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR
            | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR
            | VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR;

        VkFramebuffer   mFrameBuffer   = nullptr;
        VkPipelineCache mPipelineCache = nullptr;
        VkRenderPass    mRenderpass    = nullptr;

        core::DescriptorSetHelper mDescriptorSet;
        VkPipelineLayout          mPipelineLayout = nullptr;

        rtpipe::RtPipeline mPipeline;

        VkWriteDescriptorSetAccelerationStructureKHR               mDescriptorAccelerationStructureInfo{};
        std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> mAcclerationStructureDescriptorInfo;

        std::vector<VkDescriptorImageInfo>                         mRenderTargetDescriptorImageInfos;
        std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> mRenderTargetDescriptorInfo{};
        void                                                       UpdateRenderTargetDescriptorBufferInfos();

        struct PushConstant
        {
            uint32_t RngSeed = 0u;
        } mPushConstant;
    };
}  // namespace foray::stages