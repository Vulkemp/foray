#pragma once
#include "../basics.hpp"
#include "../core/core_declares.hpp"
#include "../mem.hpp"
#include "../stages/stages_declares.hpp"
#include "../vulkan.hpp"

namespace foray::util {
    /// @brief Wrapper for renderpass + framebuffer(s) combo
    class Renderpass
    {
      public:
        enum class AttachmentSource
        {
            Raw,
            ManagedImage,
            Swapchain
        };

        class Builder
        {
          public:
            struct Attachment
            {
                AttachmentSource                 Source;
                std::vector<VkImageView>         RawViews;
                std::vector<core::ManagedImage*> Images;
                base::VulkanWindowSwapchain*     Swapchain;
                VkImageLayout                    Layout;
                VkAttachmentDescription          Description;
            };

            Builder& AddAttachmentIn(const Attachment& attachment);
            Builder& AddAttachmentIn(core::ManagedImage* img,
                                     VkImageLayout       during,
                                     VkImageLayout       before = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM,
                                     VkImageLayout       after  = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& AddAttachmentsIn(core::ManagedImage* img,
                                      uint32_t            count,
                                      VkImageLayout       during,
                                      VkImageLayout       before = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM,
                                      VkImageLayout       after  = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& AddAttachmentOut(const Attachment& attachment);
            Builder& AddAttachmentOut(core::ManagedImage* img, VkImageLayout during, VkImageLayout after = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& AddAttachmentOut(base::VulkanWindowSwapchain* swapchain, VkImageLayout during, VkImageLayout after = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& AddAttachmentsOut(core::ManagedImage* img, uint32_t count, VkImageLayout during, VkImageLayout after = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& SetAttachmentDepthStencil(const Attachment& attachment);
            Builder& SetAttachmentDepthStencil(core::ManagedImage* img,
                                               VkImageLayout       during = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                               VkImageLayout       after  = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& ClearAttachmentDepthStencil();

            FORAY_PROPERTY_R(Input)
            FORAY_PROPERTY_R(Color)
            FORAY_GETTER_MEM(DepthStencil)

          protected:
            std::vector<Attachment> mInput;
            std::vector<Attachment> mColor;
            Local<Attachment>       mDepthStencil;
        };

        Renderpass(core::Context* context, const Builder& builder, stages::RenderDomain* domain);
        virtual ~Renderpass();

        Renderpass& UpdateImageView(uint32_t idx, VkImageView view);
        Renderpass& UpdateImageView(uint32_t idx, VkImageView* views, uint32_t count);

        uint32_t GetAttachmentCount() const { return (uint32_t)mAttachments.size(); }

      protected:
        void RecreateFramebuffer(VkExtent2D size);

        core::Context*             mContext    = nullptr;
        stages::RenderDomain*      mDomain     = nullptr;
        VkRenderPass               mRenderpass = nullptr;
        std::vector<VkFramebuffer> mFramebuffers;


        struct AttachmentInfo
        {
            AttachmentSource                 Source;
            std::vector<VkImageView>         RawViews;
            std::vector<core::ManagedImage*> Images;
            base::VulkanWindowSwapchain*     Swapchain;
        };

        std::vector<AttachmentInfo> mAttachments;

        event::PriorityReceiver<VkExtent2D> mOnResized;
    };
}  // namespace foray::util
