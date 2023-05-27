#pragma once
#include "../base/base_declares.hpp"
#include "../basics.hpp"
#include "../core/core_declares.hpp"
#include "../core/managedresource.hpp"
#include "../mem.hpp"
#include "../stages/stages_declares.hpp"
#include "../vulkan.hpp"

namespace foray::util {
    /// @brief Wrapper for renderpass + framebuffer(s) combo
    class Renderpass : public core::VulkanResource<VkObjectType::VK_OBJECT_TYPE_RENDER_PASS>
    {
      public:
        enum class AttachmentSource
        {
            Bare,
            ManagedImage,
            Swapchain
        };

        struct BareAttachment
        {
            VkImage     Image;
            VkImageView View;
        };

        class Builder
        {
          public:
            struct Attachment
            {
                AttachmentSource                 Source;
                std::vector<BareAttachment>      BareRefs;
                std::vector<core::ManagedImage*> Images;
                base::VulkanWindowSwapchain*     Swapchain;
                VkImageLayout                    Layout;
                VkAttachmentDescription          Description;
                VkClearValue                     ClearValue;
            };

            Builder& AddAttachmentIn(const Attachment& attachment);
            Builder& AddAttachmentIn(core::ManagedImage* img,
                                     VkImageLayout       during,
                                     VkImageLayout       before = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM,
                                     VkImageLayout       after  = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& AddAttachmentsIn(core::ManagedImage** img,
                                      uint32_t             count,
                                      VkImageLayout        during,
                                      VkImageLayout        before = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM,
                                      VkImageLayout        after  = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& AddAttachmentColor(const Attachment& attachment);
            Builder& AddAttachmentColorW(core::ManagedImage* img, VkImageLayout during, VkImageLayout after = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& AddAttachmentColorW(base::VulkanWindowSwapchain* swapchain, VkImageLayout during, VkImageLayout after = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& AddAttachmentsColorW(core::ManagedImage** img, uint32_t count, VkImageLayout during, VkImageLayout after = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& AddAttachmentColorRW(core::ManagedImage* img, VkImageLayout during, VkImageLayout before, VkImageLayout after = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& AddAttachmentColorRW(base::VulkanWindowSwapchain* swapchain,
                                          VkImageLayout                during,
                                          VkImageLayout                before,
                                          VkImageLayout                after = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& AddAttachmentsColorRW(
                core::ManagedImage** img, uint32_t count, VkImageLayout during, VkImageLayout before, VkImageLayout after = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& SetAttachmentDepthStencil(const Attachment& attachment);
            Builder& SetAttachmentDepthStencil(core::ManagedImage* img,
                                               VkImageLayout       during = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                               VkImageLayout       after  = VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM);
            Builder& RemoveAttachmentDepthStencil();

            FORAY_PROPERTY_R(Input)
            FORAY_PROPERTY_R(Color)
            FORAY_GETTER_MEM(DepthStencil)
            FORAY_PROPERTY_V(InitialSize)

          protected:
            std::vector<Attachment> mInput;
            std::vector<Attachment> mColor;
            Local<Attachment>       mDepthStencil;
            VkExtent2D              mInitialSize;
        };

        Renderpass(core::Context* context, const Builder& builder);
        virtual ~Renderpass();

        Renderpass& UpdateBareRef(uint32_t idx, BareAttachment bareRef);
        Renderpass& UpdateBareRefs(uint32_t idx, BareAttachment* bareRefs, uint32_t count);

        uint32_t GetAttachmentCount() const { return (uint32_t)mAttachments.size(); }

        void CmdBeginRenderpass(VkCommandBuffer cmdBuffer, uint64_t frameBufIdx);
        void CmdEndRenderpass(VkCommandBuffer cmdBuffer, core::ImageLayoutCache& layoutCache);

        FORAY_GETTER_V(Renderpass)
        FORAY_GETTER_CR(Framebuffers)
        FORAY_GETTER_V(Extent)
        FORAY_GETTER_CR(Attachments)
        FORAY_GETTER_R(ClearValues)

        void ResizeFramebuffers(VkExtent2D size);

      protected:
        core::Context*             mContext    = nullptr;
        VkRenderPass               mRenderpass = nullptr;
        std::vector<VkFramebuffer> mFramebuffers;
        VkExtent2D                 mExtent = {};


        struct AttachmentInfo
        {
            AttachmentSource                 Source;
            std::vector<BareAttachment>      BareRefs;
            std::vector<core::ManagedImage*> Images;
            base::VulkanWindowSwapchain*     Swapchain;
        };

        std::vector<AttachmentInfo> mAttachments;
        std::vector<VkClearValue>   mClearValues;

        static const uint32_t FRAMEBUFFERIDX_INVALID = ~0u;

        uint32_t                   mCurrentFrameBufferIdx = FRAMEBUFFERIDX_INVALID;
        std::vector<VkImageLayout> mPostRenderStates;
    };
}  // namespace foray::util
