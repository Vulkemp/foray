#pragma once
#include "../memory/hsk_managedimage.hpp"
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

        virtual void OnResized(const VkExtent2D& extent) override;
        virtual void Destroy() override;

      protected:
        VkFramebuffer         mFrameBuffer         = nullptr;
        VkPipelineCache       mPipelineCache       = nullptr;
        VkRenderPass          mRenderpass          = nullptr;
        VkPipeline            mPipeline            = nullptr;
        VkPipelineLayout      mPipelineLayout      = nullptr;

        virtual void CreateFixedSizeComponents(){};
        virtual void DestroyFixedComponents(){};
        virtual void CreateResolutionDependentComponents(){};
        virtual void DestroyResolutionDependentComponents(){};
    };
}  // namespace hsk
