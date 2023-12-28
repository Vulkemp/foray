#pragma once
#include "../util/descriptorsetsimple.hpp"
#include "rasterpostprocess.hpp"

namespace foray::stages {
    class TonemapStage : public RasterPostProcessBase
    {
      public:
        TonemapStage(core::Context* context, core::Image* input, vk::ImageView autoExposureImg = nullptr, int32_t resizeOrder = 0);

        void SetAutoExposureImage(vk::ImageView autoExposureImg);

        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        virtual void OnResized(VkExtent2D extent) override;

        void ImguiSetup();

        enum class EMode : uint32_t
        {
            Passthrough,
            ACES,
            AMD,
            Reinhard
        };

        void   SetMode(EMode mode);
        EMode  GetMode() const;
        void   SetExposure(fp32_t exposure);
        fp32_t GetExposure() const;

      protected:
        void LoadShader();
        void CreateSamplers();
        void CreateDescriptorSet();
        void CreateRenderpass();
        void CreatePipelineLayout();
        void CreatePipeline();

        virtual void ReloadShaders() override;

        struct PushC
        {
            uint32_t TonemapperIdx = 1u;
            fp32_t   Exposure      = 1.f;
        } mPushC;


        util::DescriptorSetSimple  mDescriptorSet;
        core::Image*        mInput;
        core::CombinedImageSampler mInputSampled;
        vk::ImageView                mAutoExposureImage;
        core::SamplerReference     mAutoExposureSampler;
    };
}  // namespace foray::stages
