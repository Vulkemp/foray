#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include "../memory/hsk_managedimage.hpp"
#include "../rtpipeline/hsk_rtpipeline.hpp"
#include "../scenegraph/hsk_scene.hpp"
#include "../utility/hsk_shadermodule.hpp"
#include "hsk_rasterizedRenderStage.hpp"

// heavily inspired by https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/raytracing_basic/raytracing_basic.cpp
namespace hsk {

    class RaytracingStage : public RenderStage
    {
      public:
        RaytracingStage() = default;

        virtual void Init();
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;

        inline static constexpr std::string_view RaytracingRenderTargetName = "RaytraycingRenderTarget";

        virtual void OnResized(const VkExtent2D& extent) override;

      protected:
        Scene*                                     mScene;
        std::vector<VkClearValue>                  mClearValues;
        std::vector<std::unique_ptr<ManagedImage>> mGBufferImages;

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

        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetAccelerationStructureDescriptorInfo(bool rebuild = false);
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetRenderTargetDescriptorInfo(bool rebuild = false);

        /// @brief Storage image that the ray generation shader will be writing to.
        ManagedImage mRaytracingRenderTarget;

        struct SampledImage
        {
            bool                                                 IsSet   = false;
            ManagedImage*                                        Image   = nullptr;
            VkSampler                                            Sampler = nullptr;
            std::vector<VkDescriptorImageInfo>                   DescriptorImageInfos;
            std::shared_ptr<DescriptorSetHelper::DescriptorInfo> DescriptorInfo{};

            inline SampledImage() {}

            void                                                 Create(const VkContext* context, ManagedImage* image, bool initateSampler = true);
            void                                                 Destroy(const VkContext* context);
            void                                                 UpdateDescriptorInfos();
            std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetDescriptorInfo(bool rebuild = false);
        } mEnvMap, mNoiseSource;

        inline static constexpr VkShaderStageFlags RTSTAGEFLAGS =
            VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR
            | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR
            | VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR;

        VkFramebuffer   mFrameBuffer   = nullptr;
        VkPipelineCache mPipelineCache = nullptr;
        VkRenderPass    mRenderpass    = nullptr;

        RtPipeline mPipeline;

        VkWriteDescriptorSetAccelerationStructureKHR         mDescriptorAccelerationStructureInfo{};
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> mAcclerationStructureDescriptorInfo;

        std::vector<VkDescriptorImageInfo>                   mRenderTargetDescriptorImageInfos;
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> mRenderTargetDescriptorInfo{};
        void                                                 UpdateRenderTargetDescriptorBufferInfos();

        struct PushConstant
        {
            uint32_t RngSeed = 0u;
        } mPushConstant;
    };
}  // namespace hsk