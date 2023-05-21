#include "renderpass.hpp"
#include "../core/managedimage.hpp"

namespace foray::util {
    Renderpass::Builder& Renderpass::Builder::AddAttachmentIn(const Attachment& attachment)
    {
        mInput.push_back(attachment);
        return *this;
    }

    Renderpass::Builder& Renderpass::Builder::AddAttachmentIn(core::ManagedImage* img, VkImageLayout during, VkImageLayout before, VkImageLayout after)
    {
        if(before == VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM)
        {
            before = during;
        }
        if(after == VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM)
        {
            after = during;
        }
        bool       discardAfterRead = after == VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        Attachment attachment{.Source      = AttachmentSource::ManagedImage,
                              .Images      = {img},
                              .Layout      = during,
                              .Description = VkAttachmentDescription{
                                  .flags          = 0,
                                  .format         = img->GetFormat(),
                                  .samples        = img->GetSampleCount(),
                                  .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD,
                                  .storeOp        = discardAfterRead ? VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE : VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_NONE,
                                  .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                  .finalLayout    = after,
                              }};
        return AddAttachmentIn(attachment);
    }

    Renderpass::Builder& Renderpass::Builder::AddAttachmentsIn(core::ManagedImage* img, uint32_t count, VkImageLayout during, VkImageLayout before, VkImageLayout after)
    {
        if(before == VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM)
        {
            before = during;
        }
        if(after == VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM)
        {
            after = during;
        }
        bool       discardAfterRead = after == VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        Attachment attachment{.Source      = AttachmentSource::ManagedImage,
                              .Images      = std::vector<core::ManagedImage*>(count),
                              .Layout      = during,
                              .Description = VkAttachmentDescription{
                                  .flags          = 0,
                                  .format         = img->GetFormat(),
                                  .samples        = img->GetSampleCount(),
                                  .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD,
                                  .storeOp        = discardAfterRead ? VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE : VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_NONE,
                                  .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                  .finalLayout    = after,
                              }};
        memcpy(attachment.Images.data(), img, count * sizeof(core::ManagedImage*));
        return AddAttachmentIn(attachment);
    }

    Renderpass::Builder& Renderpass::Builder::AddAttachmentOut(const Attachment& attachment)
    {
        mColor.push_back(attachment);
        return *this;
    }

    Renderpass::Builder& Renderpass::Builder::AddAttachmentOut(core::ManagedImage* img, VkImageLayout during, VkImageLayout after)
    {
        if(after == VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM)
        {
            after = during;
        }
        Attachment attachment{.Source      = AttachmentSource::ManagedImage,
                              .Images      = {img},
                              .Layout      = during,
                              .Description = VkAttachmentDescription{
                                  .flags          = 0,
                                  .format         = img->GetFormat(),
                                  .samples        = img->GetSampleCount(),
                                  .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                  .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                  .finalLayout    = after,
                              }};
        return AddAttachmentOut(attachment);
    }

    Renderpass::Builder& Renderpass::Builder::AddAttachmentOut(base::VulkanWindowSwapchain* swapchain, VkImageLayout during, VkImageLayout after)
    {
        if(after == VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM)
        {
            after = during;
        }
        Attachment attachment{.Source      = AttachmentSource::Swapchain,
                              .Swapchain   = swapchain,
                              .Layout      = during,
                              .Description = VkAttachmentDescription{
                                  .flags          = 0,
                                  .format         = swapchain->GetSwapchain().image_format,
                                  .samples        = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
                                  .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                  .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                  .finalLayout    = after,
                              }};
        return AddAttachmentOut(attachment);
    }

    Renderpass::Builder& Renderpass::Builder::AddAttachmentsOut(core::ManagedImage* img, uint32_t count, VkImageLayout during, VkImageLayout after)
    {
        if(after == VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM)
        {
            after = during;
        }
        bool       discardAfterRead = after == VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        Attachment attachment{.Source      = AttachmentSource::ManagedImage,
                              .Images      = std::vector<core::ManagedImage*>(count),
                              .Layout      = during,
                              .Description = VkAttachmentDescription{
                                  .flags          = 0,
                                  .format         = img->GetFormat(),
                                  .samples        = img->GetSampleCount(),
                                  .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                  .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                  .finalLayout    = after,
                              }};
        memcpy(attachment.Images.data(), img, count * sizeof(core::ManagedImage*));
        return AddAttachmentIn(attachment);
    }

    Renderpass::Builder& Renderpass::Builder::SetAttachmentDepthStencil(const Attachment& attachment)
    {
        mDepthStencil.New(attachment);
        return *this;
    }

    Renderpass::Builder& Renderpass::Builder::SetAttachmentDepthStencil(core::ManagedImage* img, VkImageLayout during, VkImageLayout after)
    {
        if(after == VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM)
        {
            after = during;
        }
        return SetAttachmentDepthStencil(Attachment{.Source      = AttachmentSource::ManagedImage,
                                                    .Images      = {img},
                                                    .Layout      = during,
                                                    .Description = VkAttachmentDescription{
                                                        .flags          = 0,
                                                        .format         = img->GetFormat(),
                                                        .samples        = img->GetSampleCount(),
                                                        .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                        .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                                        .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                        .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                        .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                                        .finalLayout    = after,
                                                    }});
    }

    Renderpass::Builder& Renderpass::Builder::ClearAttachmentDepthStencil()
    {
        mDepthStencil.Delete();
        return *this;
    }


    Renderpass::Renderpass(core::Context* context, const Builder& builder, stages::RenderDomain* domain) : mContext(context), mDomain(domain)
    {

        std::vector<VkAttachmentReference>   inputAttachmentRefs;
        std::vector<VkAttachmentReference>   colorAttachmentRefs;
        uint32_t                             colorIdx           = VK_ATTACHMENT_UNUSED;
        VkAttachmentReference                depthAttachmentRef = VkAttachmentReference{.attachment = VK_ATTACHMENT_UNUSED};
        uint32_t                             depthIdx           = VK_ATTACHMENT_UNUSED;
        std::vector<VkAttachmentDescription> attachmentDescr;

        {  // Prepare attachment member
            uint32_t attachmentCount = 0;
            attachmentCount += (uint32_t)builder.GetInput().size();
            attachmentCount += (uint32_t)builder.GetColor().size();
            attachmentCount += builder.GetDepthStencil() ? 1u : 0u;
            mAttachments.reserve(attachmentCount);
        }

        {  // Input attachments

            for(const Builder::Attachment& builderAttachment : builder.GetInput())
            {
                inputAttachmentRefs.push_back(VkAttachmentReference{.attachment = (uint32_t)attachmentDescr.size(), .layout = builderAttachment.Layout});
                attachmentDescr.push_back(builderAttachment.Description);
                AttachmentInfo info{
                    .Source = builderAttachment.Source, .RawViews = builderAttachment.RawViews, .Images = builderAttachment.Images, .Swapchain = builderAttachment.Swapchain};
                mAttachments.push_back(info);
            }
        }

        {  // Color attachments
            colorIdx = (uint32_t)attachmentDescr.size();

            for(const Builder::Attachment& builderAttachment : builder.GetColor())
            {
                colorAttachmentRefs.push_back(VkAttachmentReference{.attachment = (uint32_t)attachmentDescr.size(), .layout = builderAttachment.Layout});
                attachmentDescr.push_back(builderAttachment.Description);
                AttachmentInfo info{
                    .Source = builderAttachment.Source, .RawViews = builderAttachment.RawViews, .Images = builderAttachment.Images, .Swapchain = builderAttachment.Swapchain};
                mAttachments.push_back(info);
            }
        }

        if(builder.GetDepthStencil())
        {  // Depth / Stencil attachment
            const Builder::Attachment& builderAttachment = *builder.GetDepthStencil();

            depthIdx           = (uint32_t)colorAttachmentRefs.size();
            depthAttachmentRef = VkAttachmentReference{.attachment = depthIdx, .layout = builderAttachment.Layout};
            attachmentDescr.push_back(builderAttachment.Description);
            AttachmentInfo info{
                .Source = builderAttachment.Source, .RawViews = builderAttachment.RawViews, .Images = builderAttachment.Images, .Swapchain = builderAttachment.Swapchain};
            mAttachments.push_back(info);
        }

        VkSubpassDescription subpassDescr;
        VkSubpassDependency  subpassDeps[2];

        {  // Subpasses
            subpassDescr = VkSubpassDescription{
                .flags                   = 0,
                .pipelineBindPoint       = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount    = 0,
                .pInputAttachments       = nullptr,
                .colorAttachmentCount    = (uint32_t)colorAttachmentRefs.size(),
                .pColorAttachments       = colorAttachmentRefs.data(),
                .pResolveAttachments     = nullptr,  // multisampling stuff
                .pDepthStencilAttachment = &depthAttachmentRef,
                .preserveAttachmentCount = 0,        // relevant for multi-pass renderpasses
                .pPreserveAttachments    = nullptr,
            };

            VkPipelineStageFlags beforeStageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            VkAccessFlags        beforeAccessFlags;

            VkPipelineStageFlags subpassStageFlags;
            VkAccessFlags        subpassAccessFlags;

            VkPipelineStageFlags afterStageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            VkAccessFlags        afterAccessFlags;

            if(inputAttachmentRefs.size() > 0)
            {
                beforeAccessFlags |= VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
                subpassAccessFlags |= VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;  // The input attachment may be depth/stencil
                subpassAccessFlags |= VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                afterAccessFlags |= VkAccessFlagBits::VK_ACCESS_MEMORY_WRITE_BIT;
            }

            if(colorAttachmentRefs.size() > 0)
            {
                beforeAccessFlags |= VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT | VkAccessFlagBits::VK_ACCESS_MEMORY_WRITE_BIT;
                subpassStageFlags |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                subpassAccessFlags |= VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                afterAccessFlags |= VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT | VkAccessFlagBits::VK_ACCESS_MEMORY_WRITE_BIT;
            }

            if(depthIdx != VK_ATTACHMENT_UNUSED)
            {
                beforeAccessFlags |= VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT | VkAccessFlagBits::VK_ACCESS_MEMORY_WRITE_BIT;
                subpassStageFlags |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                subpassStageFlags |= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                subpassAccessFlags |= VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                subpassAccessFlags |= VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                afterAccessFlags |= VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT | VkAccessFlagBits::VK_ACCESS_MEMORY_WRITE_BIT;
            }

            subpassDeps[0] = VkSubpassDependency{.srcSubpass      = VK_SUBPASS_EXTERNAL,
                                                 .dstSubpass      = 0,
                                                 .srcStageMask    = beforeStageFlags,
                                                 .dstStageMask    = subpassStageFlags,
                                                 .srcAccessMask   = beforeAccessFlags,
                                                 .dstAccessMask   = subpassAccessFlags,
                                                 .dependencyFlags = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT};
            subpassDeps[1] = VkSubpassDependency{.srcSubpass      = 0,
                                                 .dstSubpass      = VK_SUBPASS_EXTERNAL,
                                                 .srcStageMask    = subpassStageFlags,
                                                 .dstStageMask    = afterStageFlags,
                                                 .srcAccessMask   = subpassAccessFlags,
                                                 .dstAccessMask   = afterAccessFlags,
                                                 .dependencyFlags = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT};
        }

        VkRenderPassCreateInfo renderpassCi{.sType           = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                            .pNext           = nullptr,
                                            .flags           = 0,
                                            .attachmentCount = (uint32_t)attachmentDescr.size(),
                                            .pAttachments    = attachmentDescr.data(),
                                            .subpassCount    = 1u,
                                            .pSubpasses      = &subpassDescr,
                                            .dependencyCount = 2u,
                                            .pDependencies   = subpassDeps};

        AssertVkResult(vkCreateRenderPass(mContext->VkDevice(), &renderpassCi, nullptr, &mRenderpass));

        RecreateFramebuffer(mDomain->GetExtent());
    }

    Renderpass::~Renderpass() 
    {
        for(VkFramebuffer framebuf : mFramebuffers)
        {
            vkDestroyFramebuffer(mContext->VkDevice(), framebuf, nullptr);
        }
        vkDestroyRenderPass(mContext->VkDevice(), mRenderpass, nullptr);
    }

    Renderpass& Renderpass::UpdateImageView(uint32_t idx, VkImageView view) 
    {
        return UpdateImageView(idx, &view, 1u);
    }

    Renderpass& Renderpass::UpdateImageView(uint32_t idx, VkImageView* views, uint32_t count)
    {
        mAttachments[idx].RawViews.resize(count);
        memcpy(mAttachments[idx].RawViews.data(), views, count);
        return *this;
    }

    void Renderpass::RecreateFramebuffer(VkExtent2D size)
    {
        for(VkFramebuffer framebuf : mFramebuffers)
        {
            vkDestroyFramebuffer(mContext->VkDevice(), framebuf, nullptr);
        }
        mFramebuffers.clear();

        uint32_t framebufferCount = 0;

        for(uint32_t idx = 0; idx < (uint32_t)mAttachments.size(); idx++)
        {
            AttachmentInfo& info = mAttachments[idx];
            switch(info.Source)
            {
                case AttachmentSource::Raw:
                    framebufferCount = std::max(framebufferCount, (uint32_t)info.RawViews.size());
                    break;
                case AttachmentSource::ManagedImage:
                    framebufferCount = std::max(framebufferCount, (uint32_t)info.Images.size());
                    break;
                case AttachmentSource::Swapchain:
                    std::vector<core::SwapchainImageInfo>& swapImgs = info.Swapchain->GetSwapchainImages();
                    framebufferCount                                = std::max(framebufferCount, (uint32_t)swapImgs.size());
                    break;
            }
        }

        mFramebuffers.resize(framebufferCount);

        for(uint32_t fbIdx = 0; fbIdx < framebufferCount; fbIdx++)
        {
            std::vector<VkImageView> attachmentViews;

            for(uint32_t idx = 0; idx < (uint32_t)mAttachments.size(); idx++)
            {
                AttachmentInfo& info = mAttachments[idx];
                VkImageView     view = nullptr;
                switch(info.Source)
                {
                    case AttachmentSource::Raw:
                        view = info.RawViews[fbIdx % (uint32_t)info.RawViews.size()];
                        break;
                    case AttachmentSource::ManagedImage:
                        view = info.Images[fbIdx % (uint32_t)info.Images.size()]->GetImageView();
                        break;
                    case AttachmentSource::Swapchain:
                        std::vector<core::SwapchainImageInfo>& swapImgs = info.Swapchain->GetSwapchainImages();
                        view                                            = swapImgs[fbIdx % swapImgs.size()];
                        break;
                }
                attachmentViews.push_back(view);
            }

            VkFramebufferCreateInfo fbufCreateInfo = {.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                                      .pNext           = nullptr,
                                                      .renderPass      = mRenderpass,
                                                      .pAttachments    = attachmentViews.data(),
                                                      .attachmentCount = (uint32_t)attachmentViews.size(),
                                                      .width           = mDomain->GetExtent().width,
                                                      .height          = mDomain->GetExtent().height,
                                                      .layers          = 1};
            AssertVkResult(vkCreateFramebuffer(mContext->VkDevice(), &fbufCreateInfo, nullptr, mFramebuffers.data() + fbIdx));
        }
    }

}  // namespace foray::util
