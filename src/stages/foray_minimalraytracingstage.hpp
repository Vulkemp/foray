#pragma once
#include "../core/foray_core_declares.hpp"
#include "../rtpipe/foray_rtpipeline.hpp"
#include "../scene/foray_scene_declares.hpp"
#include "../util/foray_pipelinelayout.hpp"
#include "foray_raytracingshared.hpp"
#include "foray_renderstage.hpp"

namespace foray::stages {
    /// @brief Minimalist setup for a Raytracing Stage
    /// @details
    /// # Features
    ///  * Pipeline & Pipeline Layout Members
    ///  * Rerouting of Init, RecordFrame, OnResized and Destroy callbacks to appropriate member methods
    /// # Inheriting
    ///  * Required Override: ApiCreatePipelineLayout(), ApiCreateRtPipeline(), ApiDestroyRtPipeline(), ApiRecordFrameBind(), ApiRecordFrameTraceRays()
    ///  * Recommended Override: ApiCreateOrUpdateDescriptors(), ApiDestroyDescriptors()
    class MinimalRaytracingStageBase : public RenderStage
    {
      public:
        /// @brief Destroys, assigns context, calls ApiCreateOutputImages(), ApiCustomObjectsCreate(), ApiCreateOrUpdateDescriptors(), ApiCreatePipelineLayout(), ApiCreateRtPipeline() in this order
        void Init(core::Context* context, RenderDomain* domain, int32_t resizeOrder = 0);

        /// @brief Calls ApiRecordFramePrepare(), ApiRecordFrameBind(), ApiRecordFrameTraceRays() in this order
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        /// @brief Calls ApiDestroyRtPipeline(), mPipelineLayout.Destroy(), ApiDestroyDescriptors(), ApiCustomObjectsDestroy() and DestroyOutputImages() in this order
        virtual void Destroy() override;

      protected:
        /// @brief Inheriting types should setup their Output Images here and push them onto the RenderStage::mImageOutput collection
        virtual void ApiCreateOutputImages() {}

        /// @brief Inheriting types should create mPipelineLayout here (add descriptorsetlayouts, add pushconstants, build)
        virtual void ApiCreatePipelineLayout() = 0;
        /// @brief Inheriting types should create mPipeline here (load shaders, configure sbts, build)
        virtual void ApiCreateRtPipeline() = 0;
        /// @brief Destroys mPipeline and all shaders registered to RenderStage::mShaders
        virtual void ApiDestroyRtPipeline() = 0;

        /// @brief Inheriting types may use this function to initialize stage specific objects such as configuration Ubo buffers
        virtual void ApiCustomObjectsCreate() {}
        /// @brief Inheriting types may use this function to destroy options created during ApiCustomObjectsCreate()
        virtual void ApiCustomObjectsDestroy() {}

        /// @brief Inheriting types should reassign all descriptor bindings and call create / update on descriptor sets
        virtual void ApiCreateOrUpdateDescriptors() {}
        /// @brief Inheriting types should destroy all descriptor sets used here
        virtual void ApiDestroyDescriptors() {}

        /// @brief Inheriting types should use this for pipeline barriers and stage specific buffer actions
        virtual void ApiRecordFramePrepare(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) {}
        /// @brief Inheriting types should use this to bind the RtPipeline and all descriptor sets
        virtual void ApiRecordFrameBind(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) = 0;
        /// @brief Inheriting types should use this to push constants and invoke tracerays
        virtual void ApiRecordFrameTraceRays(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) = 0;

        /// @brief Calls RenderStage::Resize(..) which resizes any image registered to mImageOutputs, calls ApiCreateOrUpdateDescriptors() afterwards.
        /// @param extent New render extent
        virtual void OnResized(VkExtent2D extent) override;

        /// @brief Calls ApiDestroyRtPipeline(), ApiCreateRtPipeline() in this order
        virtual void ReloadShaders() override;

        /// @brief The pipeline layout manages descriptorset and pushconstant layouts
        util::PipelineLayout mPipelineLayout;
        /// @brief The pipeline manages shader binding tables
        rtpipe::RtPipeline mPipeline;
    };

}  // namespace foray::stages