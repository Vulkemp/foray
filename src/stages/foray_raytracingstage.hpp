#pragma once
#include "../core/foray_core_declares.hpp"
#include "../rtpipe/foray_rtpipeline.hpp"
#include "../scene/foray_scene_declares.hpp"
#include "../util/foray_pipelinelayout.hpp"
#include "foray_renderstage.hpp"

// heavily inspired by https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/raytracing_basic/raytracing_basic.cpp
namespace foray::stages {

    namespace rtbindpoints {
        /// @brief Top Level Acceleration Structure Bind Point
        const uint32_t BIND_TLAS = 0;
        /// @brief Output Storage Image Bind Point
        const uint32_t BIND_OUT_IMAGE = 1;
        /// @brief Camera Ubo Buffer Bind Point
        const uint32_t BIND_CAMERA_UBO = 2;
        /// @brief Vertex Buffer Bind Point
        const uint32_t BIND_VERTICES = 3;
        /// @brief Index Buffer Bind Point
        const uint32_t BIND_INDICES = 4;
        /// @brief Material Buffer Bind Point
        const uint32_t BIND_MATERIAL_BUFFER = 5;
        /// @brief Texture Array Bind Point
        const uint32_t BIND_TEXTURES_ARRAY = 6;
        /// @brief GeometryMeta Buffer Bind Point (provided by as::Tlas, maps Blas instances to Index Buffer Offsets and Materials)
        const uint32_t BIND_GEOMETRYMETA = 7;
        /// @brief Environmentmap Sampler Bind Point
        const uint32_t BIND_ENVMAP_SPHERESAMPLER = 9;
        /// @brief  Noise Texture Storage Image Bind Point
        const uint32_t BIND_NOISETEX = 10;
    }  // namespace rtbindpoints

    /// @brief Minimalist setup for a Raytracing Stage
    /// @details
    /// # Features
    ///  * Pipeline & Pipeline Layout Members
    ///  * Rerouting of Init, RecordFrame, OnResized and Destroy callbacks to appropriate member methods
    /// # Inheriting
    ///  * Required Override: CreatePipelineLayout(), CreateRtPipeline(), DestroyRtPipeline(), RecordFramePrepare(), RecordFrameBind(), RecordFrameTraceRays()
    ///  * Recommended Override: CreateOrUpdateDescriptors(), DestroyDescriptors()
    class BasicRaytracingStage : public RenderStage
    {
      public:
        /// @brief Destroys, assigns context, calls CreateOutputImages(), CustomObjectsCreate(), CreateOrUpdateDescriptors(), CreatePipelineLayout(), CreateRtPipeline() in this order
        void Init(core::Context* context);

        /// @brief Calls RecordFramePrepare(), RecordFrameBind(), RecordFrameTraceRays() in this order
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;
        /// @brief Calls RenderStage::Resize(..) which resizes any image registered to mImageOutputs, calls CreateOrUpdateDescriptors() afterwards.
        /// @param extent New render extent
        virtual void Resize(const VkExtent2D& extent) override;

        /// @brief Calls DestroyRtPipeline(), mPipelineLayout.Destroy(), DestroyDescriptors(), CustomObjectsDestroy() DestroyOutputImages() in this order
        virtual void Destroy() override;

        /// @brief All shaderstage flags usable in a raytracing pipeline
        inline static constexpr VkShaderStageFlags RTSTAGEFLAGS =
            VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR
            | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR
            | VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR;

      protected:
        /// @brief Inheriting types should setup their Output Images here and push them onto the RenderStage::mImageOutput collection
        virtual void CreateOutputImages() {}

        /// @brief Inheriting types should create mPipelineLayout here (add descriptorsetlayouts, add pushconstants, build)
        virtual void CreatePipelineLayout() = 0;
        /// @brief Inheriting types should create mPipeline here (load shaders, configure sbts, build)
        virtual void CreateRtPipeline() = 0;
        /// @brief Destroys mPipeline and all shaders registered to RenderStage::mShaders
        virtual void DestroyRtPipeline() = 0;

        /// @brief Inheriting types may use this function to initialize stage specific objects such as configuration Ubo buffers
        virtual void CustomObjectsCreate() {}
        /// @brief Inheriting types may use this function to destroy options created during CustomObjectsCreate()
        virtual void CustomObjectsDestroy() {}

        /// @brief Inheriting types should reassign all descriptor bindings and call create / update on descriptor sets
        virtual void CreateOrUpdateDescriptors() {}
        /// @brief Inheriting types should destroy all descriptor sets used here
        virtual void DestroyDescriptors() {}

        /// @brief Inheriting types should use this for pipeline barriers and stage specific buffer actions
        virtual void RecordFramePrepare(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) = 0;
        /// @brief Inheriting types should use this to bind the RtPipeline and all descriptor sets
        virtual void RecordFrameBind(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) = 0;
        /// @brief Inheriting types should use this to push constants and invoke tracerays
        virtual void RecordFrameTraceRays(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) = 0;

        /// @brief Calls DestroyRtPipeline(), CreateRtPipeline() in this order
        virtual void ReloadShaders() override;

        /// @brief The pipeline layout manages descriptorset and pushconstant layouts
        util::PipelineLayout mPipelineLayout;
        /// @brief The pipeline manages shader binding tables
        rtpipe::RtPipeline mPipeline;
    };

    /// @brief Extended version of BasicRaytracingStage limited to a single output image, descriptorset but providing
    /// built in support for scene (Camera, Tlas, Geometry, Materials), EnvironmentMap and Noise Texture
    /// @details
    /// # Features
    ///  * Fully setup descriptorset
    ///  * Pipeline Barriers
    ///  * Binding Pipeline and DescriptorSet
    ///  * Automatically resized output image
    ///  * uint (32bit) seed value provided via push constant (offset adjustable via mRngSeedPushCOffset. Disable entirely by setting to ~0U)
    /// # Inheriting
    ///  * Required Overrides: CreateRtPipeline(), DestroyRtPipeline()
    class ExtRaytracingStage : public BasicRaytracingStage
    {
      public:
        /// @brief Nominal init for ExtRaytracingStage
        /// @param scene Scene provides Camera, Tlas, Geometry and Materials
        /// @param envMap Environment Map
        /// @param noiseImage Noise Texture
        void Init(core::Context* context, scene::Scene* scene, core::CombinedImageSampler* envMap = nullptr, core::ManagedImage* noiseImage = nullptr);

        /// @brief Image Output of the Raytracing Stage
        inline static constexpr std::string_view OutputName = "Rt.Output";

        inline core::ManagedImage* GetRtOutput() { return &mOutput; }

      protected:
        /// @brief Initializes mOutput
        /// @details mOutput initialized as rgba32f with Swapchains extent and usage flags VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        virtual void CreateOutputImages() override;

        /// @brief Creates Pipeline layout with mDescriptorSets layout and an optional single uint pushconstant (see mRngSeedPushCOffset)
        virtual void CreatePipelineLayout() override;

        /// @brief Creates a fully populated descriptorset. See rtbindpoints and shaders/rt_common/bindpoints.glsl
        virtual void CreateOrUpdateDescriptors() override;
        /// @brief Destroys the descriptor set
        virtual void DestroyDescriptors() override;

        /// @brief Pipeline barriers
        virtual void RecordFramePrepare(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;
        /// @brief Bind pipeline and descriptorset
        virtual void RecordFrameBind(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;
        /// @brief Push constant and Trace rays
        virtual void RecordFrameTraceRays(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        /// @brief Provides Camera, Tlas, Geometry and Materials
        scene::Scene* mScene = nullptr;
        /// @brief (Optional) Environment Map
        core::CombinedImageSampler* mEnvironmentMap = nullptr;
        /// @brief (Optional) Noise source image
        core::ManagedImage* mNoiseTexture = nullptr;

        /// @brief Image output
        core::ManagedImage mOutput;
        /// @brief Main DescriptorSet & Layout
        core::DescriptorSet mDescriptorSet;
        /// @brief DescriptorSet::SetDescriptorAt requires persistent .pNext objects
        VkWriteDescriptorSetAccelerationStructureKHR mDescriptorAccelerationStructureInfo{};

        /// @brief If set to anything other than ~0U a uint push constant containing the current frame idx as a seed value is added
        uint32_t mRngSeedPushCOffset = 0;
    };
}  // namespace foray::stages