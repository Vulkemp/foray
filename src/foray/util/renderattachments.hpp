#pragma once
#include "../base/base_declares.hpp"
#include "../basics.hpp"
#include "../core/core_declares.hpp"
#include "../core/managedresource.hpp"
#include "../mem.hpp"
#include "../stages/stages_declares.hpp"
#include "../vulkan.hpp"
#include <span>

namespace foray::util {
    class RenderAttachments
    {
      public:
        enum class EAttachmentBindpoint
        {
            Color,
            Depth,
            Stencil
        };

        enum class EAttachmentSource
        {
            Undefined,
            Bare,
            ManagedImage,
            Swapchain
        };

        enum class EAttachmentLoadOp
        {
            Clear,
            Load,
            Discard
        };

        struct BareAttachment
        {
            VkImage     Image;
            VkImageView View;
            VkFormat Format;
        };

        struct Attachment
        {
            EAttachmentBindpoint             Bindpoint;
            EAttachmentSource                Source;
            std::vector<BareAttachment>      BareRefs;
            std::vector<core::ManagedImage*> Images;
            base::VulkanWindowSwapchain*     Swapchain  = nullptr;
            VkImageLayout                    Layout     = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
            EAttachmentLoadOp                LoadOp     = EAttachmentLoadOp::Clear;
            bool                             Store      = true;
            VkClearValue                     ClearValue = VkClearValue{};

            Attachment();
            Attachment(EAttachmentBindpoint bindpoint, std::span<BareAttachment>, VkImageLayout layout, EAttachmentLoadOp mode, bool store, VkClearValue clearValue = {});
            Attachment(EAttachmentBindpoint bindpoint, std::span<core::ManagedImage*>, VkImageLayout layout, EAttachmentLoadOp mode, bool store, VkClearValue clearValue = {});
            Attachment(EAttachmentBindpoint bindpoint, base::VulkanWindowSwapchain*, VkImageLayout layout, EAttachmentLoadOp mode, bool store, VkClearValue clearValue = {});

            VkImage                   GetSourceImage(uint32_t resourceIdx) const;
            VkImageView               GetSourceView(uint32_t resourceIdx) const;
            VkFormat                  GetSourceFormat() const;
            VkRenderingAttachmentInfo GetAttachmentInfo(uint32_t resourceIdx) const;
            VkImageMemoryBarrier2     MakeBarrier(uint32_t resourceIdx, core::ImageLayoutCache& layoutCache) const;
        };

        RenderAttachments& SetAttachment(uint32_t idx, const Attachment& attachment);
        RenderAttachments& SetAttachmentCleared(uint32_t idx, core::ManagedImage* image, VkImageLayout layout, VkClearColorValue clearValue);
        RenderAttachments& SetAttachmentCleared(uint32_t idx, std::span<core::ManagedImage*> images, VkImageLayout layout, VkClearColorValue clearValue);
        RenderAttachments& SetAttachmentCleared(uint32_t idx, BareAttachment image, VkImageLayout layout, VkClearColorValue clearValue);
        RenderAttachments& SetAttachmentCleared(uint32_t idx, std::span<BareAttachment> images, VkImageLayout layout, VkClearColorValue clearValue);
        RenderAttachments& SetAttachmentCleared(uint32_t idx, base::VulkanWindowSwapchain* swapchain, VkImageLayout layout, VkClearColorValue clearValue);
        RenderAttachments& SetAttachmentDiscarded(uint32_t idx, core::ManagedImage* image, VkImageLayout layout);
        RenderAttachments& SetAttachmentDiscarded(uint32_t idx, std::span<core::ManagedImage*> images, VkImageLayout layout);
        RenderAttachments& SetAttachmentDiscarded(uint32_t idx, BareAttachment image, VkImageLayout layout);
        RenderAttachments& SetAttachmentDiscarded(uint32_t idx, std::span<BareAttachment> images, VkImageLayout layout);
        RenderAttachments& SetAttachmentDiscarded(uint32_t idx, base::VulkanWindowSwapchain* swapchain, VkImageLayout layout);
        RenderAttachments& SetAttachmentLoaded(uint32_t idx, core::ManagedImage* image, VkImageLayout layout);
        RenderAttachments& SetAttachmentLoaded(uint32_t idx, std::span<core::ManagedImage*> images, VkImageLayout layout);
        RenderAttachments& SetAttachmentLoaded(uint32_t idx, BareAttachment image, VkImageLayout layout);
        RenderAttachments& SetAttachmentLoaded(uint32_t idx, std::span<BareAttachment> images, VkImageLayout layout);
        RenderAttachments& SetAttachmentLoaded(uint32_t idx, base::VulkanWindowSwapchain* swapchain, VkImageLayout layout);
        RenderAttachments& AddAttachment(const Attachment& attachment);
        RenderAttachments& AddAttachmentCleared(core::ManagedImage* image, VkImageLayout layout, VkClearColorValue clearValue);
        RenderAttachments& AddAttachmentCleared(std::span<core::ManagedImage*> images, VkImageLayout layout, VkClearColorValue clearValue);
        RenderAttachments& AddAttachmentCleared(BareAttachment image, VkImageLayout layout, VkClearColorValue clearValue);
        RenderAttachments& AddAttachmentCleared(std::span<BareAttachment> images, VkImageLayout layout, VkClearColorValue clearValue);
        RenderAttachments& AddAttachmentCleared(base::VulkanWindowSwapchain* swapchain, VkImageLayout layout, VkClearColorValue clearValue);
        RenderAttachments& AddAttachmentDiscarded(core::ManagedImage* image, VkImageLayout layout);
        RenderAttachments& AddAttachmentDiscarded(std::span<core::ManagedImage*> images, VkImageLayout layout);
        RenderAttachments& AddAttachmentDiscarded(BareAttachment image, VkImageLayout layout);
        RenderAttachments& AddAttachmentDiscarded(std::span<BareAttachment> images, VkImageLayout layout);
        RenderAttachments& AddAttachmentDiscarded(base::VulkanWindowSwapchain* swapchain, VkImageLayout layout);
        RenderAttachments& AddAttachmentLoaded(core::ManagedImage* image, VkImageLayout layout);
        RenderAttachments& AddAttachmentLoaded(std::span<core::ManagedImage*> images, VkImageLayout layout);
        RenderAttachments& AddAttachmentLoaded(BareAttachment image, VkImageLayout layout);
        RenderAttachments& AddAttachmentLoaded(std::span<BareAttachment> images, VkImageLayout layout);
        RenderAttachments& AddAttachmentLoaded(base::VulkanWindowSwapchain* swapchain, VkImageLayout layout);
        RenderAttachments& SetDepthAttachmentCleared(core::ManagedImage* image, VkImageLayout layout, VkClearDepthStencilValue clearValue, bool store = true);
        RenderAttachments& SetDepthAttachmentCleared(BareAttachment image, VkImageLayout layout, VkClearDepthStencilValue clearValue, bool store = true);
        RenderAttachments& SetDepthAttachmentLoaded(core::ManagedImage* image, VkImageLayout layout, bool store = true);
        RenderAttachments& SetDepthAttachmentLoaded(BareAttachment image, VkImageLayout layout, bool store = true);
        RenderAttachments& SetStencilAttachmentCleared(core::ManagedImage* image, VkImageLayout layout, VkClearDepthStencilValue clearValue, bool store = true);
        RenderAttachments& SetStencilAttachmentCleared(BareAttachment image, VkImageLayout layout, VkClearDepthStencilValue clearValue, bool store = true);
        RenderAttachments& SetStencilAttachmentLoaded(core::ManagedImage* image, VkImageLayout layout, bool store = true);
        RenderAttachments& SetStencilAttachmentLoaded(BareAttachment image, VkImageLayout layout, bool store = true);

        FORAY_PROPERTY_R(Attachments)
        FORAY_PROPERTY_V(Flags)
        FORAY_PROPERTY_V(ViewMask)

        void CmdBeginRendering(VkCommandBuffer cmdBuffer, VkExtent2D extent, core::ImageLayoutCache& layoutCache, uint32_t resourceIdx = 0) const;

        VkPipelineRenderingCreateInfo MakePipelineRenderingCi();

      protected:
        std::vector<Attachment> mAttachments;
        Local<Attachment>       mDepthAttachment;
        Local<Attachment>       mStencilAttachment;
        VkRenderingFlags        mFlags      = 0;
        uint32_t                mLayerCount = 1;
        uint32_t                mViewMask   = 0;
        void*                   mPNext      = nullptr;
        std::vector<VkFormat>   mAttachmentFormats;
    };
}  // namespace foray::util