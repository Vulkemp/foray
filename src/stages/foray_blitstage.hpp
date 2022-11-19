#pragma once
#include "foray_renderstage.hpp"

namespace foray::stages {
    /// @brief Stage which performs an image blit (more capable copy from one image to another)
    /// @details Includes pipeline barriers
    class BlitStage : public RenderStage
    {
      public:
        BlitStage() = default;

        /// @brief Initialization
        /// @param sourceImage Source Image
        /// @param destImage Destination Image
        virtual void Init(core::ManagedImage* sourceImage = nullptr, core::ManagedImage* destImage = nullptr);
        /// @brief Records pipeline barriers for source and destination images, and the blit command itself
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        /// @brief Set source Image (nullptr allowed)
        virtual void SetSrcImage(core::ManagedImage* image);
        /// @brief Set source Image (nullptr allowed)
        virtual void SetSrcImage(VkImage image, VkExtent2D size);

        /// @brief Set destination Image (nullptr allowed)
        virtual void SetDstImage(core::ManagedImage* image);
        /// @brief Set destination Image (nullptr allowed)
        virtual void SetDstImage(VkImage image, VkExtent2D size);

        FORAY_PROPERTY_V(FlipX)
        FORAY_PROPERTY_V(FlipY)

      protected:
        /// @brief If set, preferred over mSrcVkImage member. Allows for recreated ManagedImage instances.
        core::ManagedImage* mSrcImage = nullptr;
        /// @brief Fallback to mSrcImage
        VkImage mSrcVkImage = nullptr;
        /// @brief Fallback to mSrcImage
        VkExtent2D mSrcImageSize = {};

        /// @brief If set, preferred over mDStVkImage member. Allows for recreated ManagedImage instances.
        core::ManagedImage* mDstImage = nullptr;
        /// @brief Fallback to mDstImage
        VkImage mDstVkImage = nullptr;
        /// @brief Fallback to mDstImage
        VkExtent2D mDstImageSize = {};

        /// @brief Configures the blit region
        virtual void ConfigureBlitRegion(VkImageBlit2& blit);

        /// @brief If true, flips horizontal on blitting
        bool mFlipX = false;
        /// @brief If true, flips vertically on blitting
        bool mFlipY = false;
        /// @brief Filter used for blitting
        VkFilter mFilter = VkFilter::VK_FILTER_LINEAR;
    };
}  // namespace foray::stages