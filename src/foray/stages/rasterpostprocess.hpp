#pragma once
#include "../util/pipelinelayout.hpp"
#include "../util/rasterpipeline.hpp"
#include "../util/renderpass.hpp"
#include "../core/shadermodule.hpp"
#include "renderstage.hpp"

namespace foray::stages {
    class RasterPostProcessBase : public RenderStage
    {
      public:
        RasterPostProcessBase(core::Context* context, RenderDomain* domain = nullptr, uint32_t resizeOrder = 0);

        virtual void ReloadShaders() override;

      protected:
        void CmdDraw(VkCommandBuffer cmdBuffer);
        void ConfigurePipelineBuilder();

        Local<core::ShaderModule>     mVertexShader;
        Local<core::ShaderModule>     mFragmentShader;
        Local<util::Renderpass>       mRenderpass;
        Local<util::PipelineLayout>   mPipelineLayout;
        Local<util::RasterPipeline>   mPipeline;
        util::RasterPipeline::Builder mPipelineBuilder;
    };
}  // namespace foray::stages
