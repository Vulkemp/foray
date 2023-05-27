#pragma once
#include "../util/descriptorsetsimple.hpp"
#include "../util/pipelinelayout.hpp"
#include "../util/rasterpipeline.hpp"
#include "../util/renderpass.hpp"
#include "renderstage.hpp"

namespace foray::stages {

    /// @brief Base class for rasterized render stages (no built-in functionality)
    class RasterizedRenderStage : public RenderStage
    {
      public:
        inline RasterizedRenderStage(core::Context* context = nullptr, RenderDomain* domain = nullptr, int32_t priority = 0) : RenderStage(context, domain, priority) {}

        FORAY_GETTER_MEM(Renderpass)
        FORAY_GETTER_MEM(Pipeline)
        FORAY_GETTER_MEM(PipelineLayout)

      protected:
        Local<util::Renderpass>     mRenderpass;
        Local<util::PipelineLayout> mPipelineLayout;
        Local<util::RasterPipeline> mPipeline;
    };
}  // namespace foray::stages
