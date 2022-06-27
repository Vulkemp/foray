#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "../scenegraph/hsk_scene.hpp"
#include "hsk_rasterizedRenderStage.hpp"

namespace hsk {
    class FlipImageStage : public RasterizedRenderStage
    {
      public:
        FlipImageStage() = default;

        virtual void Init(const VkContext* context, ManagedImage* sourceImage);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;

        inline static constexpr std::string_view FlipTarget = "FlipImageStage_FlipTarget";

        virtual void OnResized(const VkExtent2D& extent, ManagedImage* newSourceImage);

      protected:
        std::vector<VkClearValue>                  mClearValues;
        std::vector<std::unique_ptr<ManagedImage>> mFlipImages;
        ManagedImage*                              mSourceImage;

        virtual void CreateFixedSizeComponents() override;
        virtual void DestroyFixedComponents() override;
        virtual void CreateResolutionDependentComponents() override;
        virtual void DestroyResolutionDependentComponents() override;


        void PrepareAttachments();

    };
}  // namespace hsk