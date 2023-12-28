#pragma once
#include "../core/image.hpp"
#include "../core/samplercollection.hpp"
#include "rasterpostprocess.hpp"
#include "tonemapstage.hpp"

namespace foray::stages {
    class AverageLuminanceStage : public RasterPostProcessBase
    {
      public:
        AverageLuminanceStage(core::Context* context, RenderDomain* domain, core::ImageViewRef* input, int32_t resizeOrder = 1);

        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        virtual void OnResized(VkExtent2D extent) override;

        FORAY_GETTER_MEM(ReduceFinalMipView)

      protected:
        void ConfigureReduceImage(VkExtent2D inputExtent);
        void CreateSamplers();
        void CreateDescriptorSet();
        void CreatePipelineLayout();
        void CreatePipeline();

        virtual void ReloadShaders() override;

        std::string                mInputName;
        core::CombinedImageSampler mInputSampled;
        util::DescriptorSetSimple  mDescriptorSet;
        Local<core::Image>         mReduceImage;
        Local<core::ImageViewRef>  mReduceInitialMipView;
        Local<core::ImageViewRef>  mReduceFinalMipView;
        core::ManualRenderTarget   mReduceFinalTarget;
    };
}  // namespace foray::stages
