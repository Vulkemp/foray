#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "hsk_rasterizedRenderStage.hpp"

namespace hsk {
    class ImguiStage : public RasterizedRenderStage
    {
      public:
        ImguiStage() = default;

        virtual void Init(const VkContext* context, ManagedImage* backgroundImage);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;
        virtual void Destroy() override;

        /// @brief Add a function that renders an imgui window. Example:
        void AddWindowDraw(std::function<void()> windowDraw) { mWindowDraws.push_back(windowDraw); }

        /// @brief When the window has been resized, update the target image.
        virtual void OnResized(const VkExtent2D& extent, ManagedImage* newTargetImage);

      protected:
        std::vector<VkClearValue>          mClearValues;
        ManagedImage*                      mTargetImage{};
        VkDescriptorPool                   mImguiPool{};
        std::vector<std::function<void()>> mWindowDraws;

        virtual void CreateFixedSizeComponents();
        virtual void CreateResolutionDependentComponents();
        virtual void DestroyResolutionDependentComponents();

        void PrepareAttachments();
        void PrepareRenderpass();
        void BuildCommandBuffer(){};
    };
}  // namespace hsk