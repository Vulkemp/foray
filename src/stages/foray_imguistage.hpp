#pragma once
#include "foray_rasterizedRenderStage.hpp"
#include <sdl2/SDL.h>
#include <functional>

namespace foray::stages {
    /// @brief The ImguiStage renders the imgui menu on top of an existing image. This image is passed via the backgroundImage
    class ImguiStage : public RasterizedRenderStage
    {
      public:
        ImguiStage() = default;

        virtual void Init(core::Context* context, core::ManagedImage* backgroundImage);
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;
        virtual void Destroy() override;

        virtual void SetBackgroundImage(core::ManagedImage* backgroundImage);

        /// @brief Add a function that renders an imgui window. Example:
        void AddWindowDraw(std::function<void()> windowDraw) { mWindowDraws.push_back(windowDraw); }

        /// @brief When the window has been resized, update the target image.
        virtual void Resize(const VkExtent2D& extent) override;

        /// @brief Allows imgui to handle input events.
        void ProcessSdlEvent(const SDL_Event* sdlEvent);

      protected:
        std::vector<VkClearValue>          mClearValues;
        core::ManagedImage*                      mTargetImage{};
        VkDescriptorPool                   mImguiPool{};
        std::vector<std::function<void()>> mWindowDraws;

        virtual void DestroyFrameBufferAndRenderPass();

        void PrepareRenderpass();
    };
}  // namespace foray