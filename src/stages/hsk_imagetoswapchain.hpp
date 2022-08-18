#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "hsk_renderstage.hpp"

namespace hsk {
    /// @brief The only purpose of this class is to copy the image onto the swapchain
    class ImageToSwapchainStage : public RenderStage
    {
      public:
        ImageToSwapchainStage() = default;

        class PostCopy
        {
          public:
            VkAccessFlags AccessFlags      = 0;
            VkImageLayout ImageLayout      = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
            uint32_t      QueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        };

        virtual void Init(const VkContext* context, ManagedImage* sourceImage);
        virtual void Init(const VkContext* context, ManagedImage* sourceImage, const PostCopy& postcopy);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;
        virtual void OnResized(const VkExtent2D& extent, ManagedImage* newSourceImage);

        virtual void SetTargetImage(ManagedImage* newTargetImage);

      protected:
        ManagedImage* mSourceImage{};
        PostCopy      mPostCopy;
    };
}  // namespace hsk