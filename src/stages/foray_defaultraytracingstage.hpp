#pragma once
#include "../core/foray_core_declares.hpp"
#include "../rtpipe/foray_rtpipeline.hpp"
#include "../scene/foray_scene_declares.hpp"
#include "../util/foray_pipelinelayout.hpp"
#include "foray_raytracingshared.hpp"
#include "foray_renderstage.hpp"

namespace foray::stages {
    /// @brief Extended version of MinimalRaytracingStageBase limited to a single output image, descriptorset but providing
    /// built in support for scene (Camera, Tlas, Geometry, Materials), EnvironmentMap and Noise Texture
    /// @details
    /// # Features
    ///  * Fully setup descriptorset
    ///  * Pipeline Barriers
    ///  * Binding Pipeline and DescriptorSet
    ///  * Automatically resized output image
    ///  * uint (32bit) seed value provided via push constant (offset adjustable via mRngSeedPushCOffset. Disable entirely by setting to ~0U)
    /// # Inheriting
    ///  * Required Overrides: ApiCreateRtPipeline(), ApiDestroyRtPipeline()
    class DefaultRaytracingStageBase : public RenderStage
    {
      public:
        DefaultRaytracingStageBase(core::Context* context, RenderDomain* domain, int32_t resizeOrder = 0);

        /// @brief Nominal init for DefaultRaytracingStageBase
        /// @param scene Scene provides Camera, Tlas, Geometry and Materials
        /// @param envMap Environment Map
        /// @param noiseImage Noise Texture
        void Init(scene::Scene* scene, core::CombinedImageSampler* envMap = nullptr, core::ManagedImage* noiseImage = nullptr);

        /// @brief Calls RecordFramePrepare(), RecordFrameBind(), RecordFrameTraceRays() in this order
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        /// @brief Image Output of the Raytracing Stage
        inline static constexpr std::string_view OutputName = "Rt.Output";

        inline core::ManagedImage* GetRtOutput() { return mOutput.Get(); }

        virtual ~DefaultRaytracingStageBase();

      protected:
        /// @brief Initializes mOutput
        /// @details mOutput initialized as rgba32f with Swapchains extent and usage flags VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        virtual void CreateOutputImages();

        /// @brief Creates Pipeline layout with mDescriptorSets layout and an optional single uint pushconstant (see mRngSeedPushCOffset)
        virtual void CreatePipelineLayout();
        /// @brief Inheriting types should create mPipeline here (load shaders, configure sbts, build)
        virtual void ApiCreateRtPipeline() = 0;
        /// @brief Destroys mPipeline and all shaders registered to RenderStage::mShaders
        virtual void ApiDestroyRtPipeline() = 0;

        /// @brief Inheriting types may use this function to initialize stage specific objects such as configuration Ubo buffers
        virtual void ApiCustomObjectsCreate() {}
        /// @brief Inheriting types may use this function to destroy options created during ApiCustomObjectsCreate()
        virtual void ApiCustomObjectsDestroy() {}

        /// @brief Creates a fully populated descriptorset. See rtbindpoints and shaders/rt_common/bindpoints.glsl
        virtual void CreateOrUpdateDescriptors();

        /// @brief Calls ApiDestroyRtPipeline(), ApiCreateRtPipeline() in this order
        virtual void ReloadShaders() override;

        /// @brief Pipeline barriers
        virtual void RecordFramePrepare(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo);
        /// @brief Bind pipeline and descriptorset
        virtual void RecordFrameBind(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo);
        /// @brief Push constant and Trace rays
        virtual void RecordFrameTraceRays(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo);

        /// @brief Calls RenderStage::Resize(..) which resizes any image registered to mImageOutputs, calls CreateOrUpdateDescriptors() afterwards.
        /// @param extent New render extent
        virtual void OnResized(VkExtent2D extent) override;

        /// @brief Provides Camera, Tlas, Geometry and Materials
        scene::Scene* mScene = nullptr;
        /// @brief (Optional) Environment Map
        core::CombinedImageSampler* mEnvironmentMap = nullptr;
        /// @brief (Optional) Noise source image
        core::ManagedImage* mNoiseTexture = nullptr;

        /// @brief Image output
        Local<core::ManagedImage> mOutput;
        /// @brief Main DescriptorSet & Layout
        core::DescriptorSet mDescriptorSet;
        /// @brief DescriptorSet::SetDescriptorAt requires persistent .pNext objects
        VkWriteDescriptorSetAccelerationStructureKHR mDescriptorAccelerationStructureInfo{};

        /// @brief The pipeline layout manages descriptorset and pushconstant layouts
        Local<util::PipelineLayout> mPipelineLayout;
        /// @brief The pipeline manages shader binding tables
        Local<rtpipe::RtPipeline> mPipeline;

        /// @brief If set to anything other than ~0U a uint push constant containing the current frame idx as a seed value is added
        uint32_t mRngSeedPushCOffset = 0;
    };
}  // namespace foray::stages