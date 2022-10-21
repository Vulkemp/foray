#pragma once
#include "foray_renderstage.hpp"

namespace foray::stages {
    /// @brief The only purpose of this class is to copy the image onto the swapchain
    class ImageToSwapchainStage : public RenderStage
    {
      public:
        ImageToSwapchainStage() = default;

        virtual void Init(core::Context* context, core::ManagedImage* sourceImage);
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        virtual void SetTargetImage(core::ManagedImage* newTargetImage);

      protected:
        core::ManagedImage* mSourceImage{};
    };
}  // namespace foray::stages