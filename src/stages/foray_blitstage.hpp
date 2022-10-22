#pragma once
#include "foray_renderstage.hpp"

namespace foray::stages {
    class BlitStage : public RenderStage
    {
      public:
        BlitStage() = default;

        virtual void Init(core::Context* context, core::ManagedImage* sourceImage, core::ManagedImage* destImage);
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        virtual void SetSrcImage(core::ManagedImage* image);
        virtual void SetSrcImage(VkImage image, std::string_view name, VkExtent2D size);

        virtual void SetDstImage(core::ManagedImage* image);
        virtual void SetDstImage(VkImage image, std::string_view name, VkExtent2D size);

      protected:
        core::ManagedImage* mSrcImage_    = nullptr;
        VkImage             mSrcVkImage   = nullptr;
        std::string         mSrcImageName = "";
        VkExtent2D          mSrcImageSize = {};

        core::ManagedImage* mDstImage_    = nullptr;
        VkImage             mDstVkImage   = nullptr;
        std::string         mDstImageName = "";
        VkExtent2D          mDstImageSize = {};

        virtual void ConfigureBlitRegion(VkImageBlit2& blit);

        bool     mFlipX  = false;
        bool     mFlipY  = false;
        VkFilter mFilter = VkFilter::VK_FILTER_LINEAR;
    };
}  // namespace foray::stages