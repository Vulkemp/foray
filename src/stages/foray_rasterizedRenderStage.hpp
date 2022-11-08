#pragma once
#include "foray_renderstage.hpp"
#include "../util/foray_pipelinelayout.hpp"

namespace foray::stages {
    class RasterizedRenderStage : public RenderStage
    {
      public:
        RasterizedRenderStage() = default;

        FORAY_GETTER_CR(FrameBuffer)
        FORAY_GETTER_CR(Renderpass)
        FORAY_GETTER_CR(Pipeline)
        FORAY_GETTER_CR(PipelineLayout)

        virtual void SetupDescriptors(){};
        virtual void CreateDescriptorSets(){};
        virtual void UpdateDescriptors(){};
        virtual void CreatePipelineLayout(){};

      protected:
        VkFramebuffer        mFrameBuffer   = nullptr;
        VkRenderPass         mRenderpass    = nullptr;
        core::DescriptorSet  mDescriptorSet;
        VkPipeline           mPipeline       = nullptr;
        util::PipelineLayout mPipelineLayout;
    };
}  // namespace foray::stages
