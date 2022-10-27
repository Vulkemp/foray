#pragma once
#include "foray_renderstage.hpp"

namespace foray::stages {
    class RasterizedRenderStage : public RenderStage
    {
      public:
        RasterizedRenderStage(){};
        ~RasterizedRenderStage() { Destroy(); };

        FORAY_PROPERTY_CGET(FrameBuffer)
        FORAY_PROPERTY_CGET(PipelineCache)
        FORAY_PROPERTY_CGET(Renderpass)
        FORAY_PROPERTY_CGET(Pipeline)
        FORAY_PROPERTY_CGET(PipelineLayout)

        virtual void SetupDescriptors(){};
        virtual void CreateDescriptorSets(){};
        virtual void UpdateDescriptors(){};
        virtual void CreatePipelineLayout(){};

        virtual void OnResized(const VkExtent2D& extent) override;
        virtual void Destroy() override;

      protected:
        VkFramebuffer       mFrameBuffer   = nullptr;
        VkPipelineCache     mPipelineCache = nullptr;
        VkRenderPass        mRenderpass    = nullptr;
        core::DescriptorSet mDescriptorSet;
        VkPipeline          mPipeline       = nullptr;
        VkPipelineLayout    mPipelineLayout = nullptr;
    };
}  // namespace foray::stages
