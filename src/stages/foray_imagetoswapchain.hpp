#pragma once
#include "foray_blitstage.hpp"

namespace foray::stages {
    /// @brief The only purpose of this class is to copy the image onto the swapchain
    class ImageToSwapchainStage : public BlitStage
    {
      public:
        ImageToSwapchainStage() = default;

        void Init(core::Context* context, core::ManagedImage* srcImage);
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;
    };
}  // namespace foray::stages