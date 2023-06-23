#pragma once
#include "../core/managedimage.hpp"
#include "../core/samplercollection.hpp"
#include "rasterpostprocess.hpp"
#include "tonemapstage.hpp"

namespace foray::stages {
    class AdaptiveTonemapStage : public RasterPostProcessBase
    {
      public:
        AdaptiveTonemapStage(core::Context* context, RenderDomain* domain, core::ManagedImage* input, int32_t resizeOrder = 1);


        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        virtual void OnResized(VkExtent2D extent) override;

        virtual void SetResizeOrder(int32_t priority) override;

      protected:
        void ConfigureReduceImage(VkExtent2D inputExtent);
        void CreateSamplers();
        void CreateDescriptorSet();
        void CreatePipelineLayout();
        void CreatePipeline();

        virtual void ReloadShaders() override;

        core::ManagedImage*        mInput = nullptr;
        core::CombinedImageSampler mInputSampled;
        util::DescriptorSetSimple  mDescriptorSet;
        core::Local_ManagedImage   mReduceImage;
        VkImageView                mReduceFinalMipView = nullptr;
        Local<TonemapStage>        mTonemapStage;
    };
}  // namespace foray::stages
