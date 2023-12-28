#pragma once
#include "blitstage.hpp"

namespace foray::stages {
    /// @brief The only purpose of this class is to copy the image onto the swapchain
    class ImageToSwapchainStage : public BlitStage
    {
      public:
        ImageToSwapchainStage(core::Context* context, core::Image* srcImage);
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;
    };
}  // namespace foray::stages