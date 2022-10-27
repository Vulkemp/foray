#pragma once
#include "foray_renderstage.hpp"
#include "../util/foray_pipelinelayout.hpp"

namespace foray::stages {
    class RasterizedRenderStage : public RenderStage
    {
      public:
        RasterizedRenderStage(){};
        ~RasterizedRenderStage() { Destroy(); };

        FORAY_PROPERTY_CGET(FrameBuffer)
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
        VkFramebuffer        mFrameBuffer   = nullptr;
        VkRenderPass         mRenderpass    = nullptr;
        core::DescriptorSet  mDescriptorSet;
        VkPipeline           mPipeline       = nullptr;
        util::PipelineLayout mPipelineLayout;
    };
}  // namespace foray::stages
