#pragma once
#include "../util/foray_pipelinelayout.hpp"
#include "foray_renderstage.hpp"
#include "../util/foray_descriptorsetsimple.hpp"

namespace foray::stages {

    /// @brief Base class for rasterized render stages (no built-in functionality)
    class RasterizedRenderStage : public RenderStage
    {
      public:
        inline RasterizedRenderStage(core::Context* context = nullptr, RenderDomain* domain = nullptr, int32_t priority = 0) : RenderStage(context, domain, priority) {}

        FORAY_GETTER_CR(FrameBuffer)
        FORAY_GETTER_CR(Renderpass)
        FORAY_GETTER_CR(Pipeline)
        FORAY_GETTER_CR(PipelineLayout)

        virtual void SetupDescriptors(){};
        virtual void CreateDescriptorSets(){};
        virtual void UpdateDescriptors(){};
        virtual void CreatePipelineLayout(){};

      protected:
        VkFramebuffer               mFrameBuffer = nullptr;
        VkRenderPass                mRenderpass  = nullptr;
        util::DescriptorSetSimple   mDescriptorSet;
        VkPipeline                  mPipeline = nullptr;
        Local<util::PipelineLayout> mPipelineLayout;
    };
}  // namespace foray::stages
