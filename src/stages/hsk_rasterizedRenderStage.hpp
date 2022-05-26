#pragma once
#include "../memory/hsk_intermediateImage.hpp"
#include "hsk_renderstage.hpp"

namespace hsk {
    class RasterizedRenderStage : public RenderStage
    {
      public:
        RasterizedRenderStage(){};

        HSK_PROPERTY_CGET(FrameBuffer)
        HSK_PROPERTY_CGET(PipelineCache)
        HSK_PROPERTY_CGET(Renderpass)
        HSK_PROPERTY_CGET(Pipeline)
        HSK_PROPERTY_CGET(PipelineLayout)

      protected:

        VkFramebuffer         mFrameBuffer         = nullptr;
        VkPipelineCache       mPipelineCache       = nullptr;
        VkRenderPass          mRenderpass          = nullptr;
        VkPipeline            mPipeline            = nullptr;
        VkPipelineLayout      mPipelineLayout      = nullptr;

        virtual void Destroy() override;
        virtual void DestroyFixedComponents();
        virtual void DestroyResolutionDependentComponents();
    };
}  // namespace hsk
