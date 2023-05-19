#pragma once
#include "rasterizedRenderStage.hpp"
#include "../osi/osi_event.hpp"
#include <functional>
#include <sdl2/SDL.h>

namespace foray::stages {
    /// @brief The ImguiStage renders the imgui menu on top of an existing image or the swapchain. This image is passed via the backgroundImage
    class ImguiStage : public RenderStage
    {
      public:
        /// @brief Init the imgui stage for rendering over a generic background image
        /// @param context Requires Device
        /// @param backgroundImage Managed Image Background Image to render over
        ImguiStage(core::Context* context, RenderDomain* domain, core::ManagedImage* backgroundImage, int32_t resizeOrder = 0);
        /// @brief Init the imgui stage for rendering to the swapchain
        /// @param context Requires Device, Swapchain & SwapchainImages
        ImguiStage(core::Context* context, int32_t resizeOrder = 0);
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        /// @brief Switch background image and between modes at runtime
        /// @param backgroundImage Managed Image Background Image to render over. If set to nullptr, will use swapchain mode.
        virtual void SetBackgroundImage(core::ManagedImage* backgroundImage);

        /// @brief Add a function that renders an imgui window. Example:
        void AddWindowDraw(std::function<void()> windowDraw) { mWindowDraws.push_back(windowDraw); }

        /// @brief When the window has been resized, update the target images.
        virtual void OnResized(VkExtent2D extent) override;

        virtual ~ImguiStage();

      protected:
        VkClearValue mClearValue;

        core::ManagedImage*                mTargetImage = nullptr;
        VkDescriptorPool                   mImguiPool{};
        std::vector<std::function<void()>> mWindowDraws;

        virtual void InitImgui();
        virtual void PrepareRenderpass();
        virtual void DestroyFrameBufferAndRenderPass();

        void HandleSdlEvent(const osi::EventRawSDL* event);

        std::vector<VkFramebuffer> mFrameBuffers;
        VkRenderPass               mRenderPass = nullptr;

        event::Receiver<const osi::EventRawSDL*> mOnSdlEvent;
    };
}  // namespace foray::stages