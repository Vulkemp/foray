#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "hsk_renderstage.hpp"

namespace hsk {
    /// @brief The only purpose of this class is to copy the image onto the swapchain
    class ImageToSwapchainStage : public RenderStage
    {
      public:
        ImageToSwapchainStage() = default;

        virtual void Init(const VkContext* context, ManagedImage* sourceImage);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;
        virtual void OnResized(const VkExtent2D& extent, ManagedImage* newSourceImage);

      protected:
        ManagedImage* mSourceImage{};
    };
}  // namespace hsk