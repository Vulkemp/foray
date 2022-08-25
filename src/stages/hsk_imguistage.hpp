#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "hsk_rasterizedRenderStage.hpp"

namespace hsk {
    /// @brief The ImguiStage renders the imgui menu on top of an existing image. This image is passed via the backgroundImage
    class ImguiStage : public RasterizedRenderStage
    {
      public:
        ImguiStage() = default;

        virtual void Init(const VkContext* context, ManagedImage* backgroundImage);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;
        virtual void Destroy() override;

        virtual void SetTargetImage(ManagedImage* newTargetImage);

        /// @brief Add a function that renders an imgui window. Example:
        void AddWindowDraw(std::function<void()> windowDraw) { mWindowDraws.push_back(windowDraw); }

        /// @brief When the window has been resized, update the target image.
        virtual void OnResized(const VkExtent2D& extent, ManagedImage* newTargetImage);

        /// @brief Allows imgui to handle input events.
        void ProcessSdlEvent(const SDL_Event* sdlEvent);

      protected:
        std::vector<VkClearValue>          mClearValues;
        ManagedImage*                      mTargetImage{};
        VkDescriptorPool                   mImguiPool{};
        std::vector<std::function<void()>> mWindowDraws;

        virtual void CreateFixedSizeComponents() override;
        virtual void CreateResolutionDependentComponents() override;
        virtual void DestroyResolutionDependentComponents() override;

        void PrepareAttachments();
        void PrepareRenderpass();
        void BuildCommandBuffer(){};
    };
}  // namespace hsk