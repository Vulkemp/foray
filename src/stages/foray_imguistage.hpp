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

        virtual void SetTargetImage(core::ManagedImage* newTargetImage);

        /// @brief Add a function that renders an imgui window. Example:
        void AddWindowDraw(std::function<void()> windowDraw) { mWindowDraws.push_back(windowDraw); }

        /// @brief When the window has been resized, update the target image.
        virtual void OnResized(const VkExtent2D& extent, core::ManagedImage* newTargetImage);

        /// @brief Allows imgui to handle input events.
        void ProcessSdlEvent(const SDL_Event* sdlEvent);

      protected:
        std::vector<VkClearValue>          mClearValues;
        core::ManagedImage*                      mTargetImage{};
        VkDescriptorPool                   mImguiPool{};
        std::vector<std::function<void()>> mWindowDraws;

        virtual void CreateFixedSizeComponents() override;
        virtual void CreateResolutionDependentComponents() override;
        virtual void DestroyResolutionDependentComponents() override;

        void PrepareAttachments();
        void PrepareRenderpass();
        void BuildCommandBuffer(){};
    };
}  // namespace foray