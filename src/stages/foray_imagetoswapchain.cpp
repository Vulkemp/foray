#include "foray_imagetoswapchain.hpp"

namespace foray::stages {
    void ImageToSwapchainStage::Init(core::Context* context, core::ManagedImage* srcImage)
    {
        BlitStage::Init(context, srcImage, nullptr);
    }

    void ImageToSwapchainStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        uint32_t swapChainImageIndex = renderInfo.GetInFlightFrame()->GetSwapchainImageIndex();

        const core::SwapchainImageInfo& swapImage = mContext->SwapchainImages[swapChainImageIndex];

        VkExtent2D swapChainSize = mContext->GetSwapchainSize();

        SetDstImage(swapImage.Image, swapImage.Name, swapChainSize);

        BlitStage::RecordFrame(cmdBuffer, renderInfo);
    }

}  // namespace foray::stages