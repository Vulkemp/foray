#pragma once
#include "../memory/hsk_managedimage.hpp"
#include "hsk_renderstage.hpp"

namespace hsk {
    class RasterizedRenderStage : public RenderStage
    {
      public:
        RasterizedRenderStage(){};
        ~RasterizedRenderStage() { Destroy(); };

        HSK_PROPERTY_CGET(FrameBuffer)
        HSK_PROPERTY_CGET(PipelineCache)
        HSK_PROPERTY_CGET(Renderpass)
        HSK_PROPERTY_CGET(Pipeline)
        HSK_PROPERTY_CGET(PipelineLayout)

        virtual void OnResized(const VkExtent2D& extent) override;
        virtual void Destroy() override;

      protected:
        VkFramebuffer         mFrameBuffer         = nullptr;
        VkPipelineCache       mPipelineCache       = nullptr;
        VkRenderPass          mRenderpass          = nullptr;
    };
}  // namespace hsk
