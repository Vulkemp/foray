#include "imagetoswapchain.hpp"

namespace foray::stages {
    ImageToSwapchainStage::ImageToSwapchainStage(core::Context* context, core::Image* srcImage)
     : BlitStage(srcImage, nullptr)
    {
        mContext = context;
    }

    void ImageToSwapchainStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        uint32_t swapChainImageIndex = renderInfo.GetInFlightFrame()->GetSwapchainImageIndex();

        const core::SwapchainImageInfo& swapImage = mContext->WindowSwapchain->GetSwapchainImages()[swapChainImageIndex];

        VkExtent2D swapChainSize = mContext->WindowSwapchain->GetExtent();

        SetDstImage(swapImage.Image, swapChainSize);

        BlitStage::RecordFrame(cmdBuffer, renderInfo);
    }

}  // namespace foray::stages