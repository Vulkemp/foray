#pragma once
#include "foray_rasterizedRenderStage.hpp"
#include <functional>
#include <sdl2/SDL.h>

namespace foray::stages {
    /// @brief The ImguiStage renders the imgui menu on top of an existing image or the swapchain. This image is passed via the backgroundImage
    class ImguiStage : public RenderStage
    {
      public:
        ImguiStage() = default;

        /// @brief Initializes and selects background image mode if set, swapchain mode otherwise
        /// @param context Requires Device (Swapchain & SwapchainImages if no background image is set)
        /// @param backgroundImage Managed Image Background Image to render over. If set to nullptr, will use swapchain mode.
        virtual void Init(core::Context* context, core::ManagedImage* backgroundImage);
        /// @brief Init the imgui stage for rendering over a generic background image
        /// @param context Requires Device
        /// @param backgroundImage Managed Image Background Image to render over
        virtual void InitForImage(core::Context* context, core::ManagedImage* backgroundImage);
        /// @brief Init the imgui stage for rendering to the swapchain
        /// @param context Requires Device, Swapchain & SwapchainImages
        virtual void InitForSwapchain(core::Context* context);
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;
        virtual void Destroy() override;

        /// @brief Switch background image and between modes at runtime
        /// @param backgroundImage Managed Image Background Image to render over. If set to nullptr, will use swapchain mode.
        virtual void SetBackgroundImage(core::ManagedImage* backgroundImage);

        /// @brief Add a function that renders an imgui window. Example:
        void AddWindowDraw(std::function<void()> windowDraw) { mWindowDraws.push_back(windowDraw); }

        /// @brief When the window has been resized, update the target images.
        virtual void Resize(const VkExtent2D& extent) override;

        /// @brief Allows imgui to handle input events.
        void ProcessSdlEvent(const SDL_Event* sdlEvent);

        inline virtual ~ImguiStage() { Destroy(); }

      protected:
        VkClearValue mClearValue;

        core::ManagedImage*                mTargetImage = nullptr;
        VkDescriptorPool                   mImguiPool{};
        std::vector<std::function<void()>> mWindowDraws;

        virtual void InitImgui();
        virtual void PrepareRenderpass();
        virtual void DestroyFrameBufferAndRenderPass();


        std::vector<VkFramebuffer> mFrameBuffers;
        VkRenderPass               mRenderPass = nullptr;
    };
}  // namespace foray::stages