#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "hsk_rasterizedRenderStage.hpp"

namespace hsk {
    class ImguiStage : public RasterizedRenderStage
    {
      public:
        ImguiStage() = default;

        virtual void Init(const VkContext* context);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;
        virtual void Destroy();

        /// @brief Add a function that renders an imgui window. Example:
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