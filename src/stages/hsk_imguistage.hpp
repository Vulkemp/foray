#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "hsk_rasterizedRenderStage.hpp"

namespace hsk {
    class ImguiStage : public RasterizedRenderStage
    {
      public:
        ImguiStage() = default;
        virtual ~ImguiStage() { Destroy(); }

        virtual void Init(const VkContext* context, ManagedImage* backgroundImage);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;
        virtual void Destroy();

        void OnResized(const VkExtent2D& extent, ManagedImage* newBackgroundImage);

        /// @brief Set the window that Imgui will draw onto
        virtual void SetBackgroundImage(ManagedImage* backgroundImage) { mBackgroundImage = backgroundImage; }

        /// @brief Add a function that renders an imgui window. Example:
        ///
        void AddWindowDraw(std::function<void()> windowDraw) { mWindowDraws.push_back(windowDraw); }

      protected:
        std::vector<VkClearValue>          mClearValues;
        ManagedImage*                      mBackgroundImage{};
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