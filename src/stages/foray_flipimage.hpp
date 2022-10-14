#pragma once
#include "../core/foray_vkcontext.hpp"
//#include "../scene/foray_scene.hpp"
#include "foray_rasterizedRenderStage.hpp"

namespace foray::stages {
    class FlipImageStage : public RasterizedRenderStage
    {
      public:
        FlipImageStage() = default;

        virtual void Init(const core::VkContext* context, core::ManagedImage* sourceImage);
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        inline static constexpr std::string_view FlipTarget = "Flip.Target";

        virtual void OnResized(const VkExtent2D& extent, core::ManagedImage* newSourceImage);

      protected:
        std::vector<VkClearValue>                        mClearValues;
        std::vector<std::unique_ptr<core::ManagedImage>> mFlipImages;
        core::ManagedImage*                              mSourceImage;

        virtual void CreateFixedSizeComponents() override;
        virtual void DestroyFixedComponents() override;
        virtual void CreateResolutionDependentComponents() override;
        virtual void DestroyResolutionDependentComponents() override;


        void PrepareAttachments();
    };
}  // namespace foray::stages