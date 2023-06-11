#include "renderpass.hpp"
#include "../core/imagelayoutcache.hpp"
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

    Renderpass::Builder& Renderpass::Builder::AddAttachmentsIn(core::ManagedImage** img, uint32_t count, VkImageLayout during, VkImageLayout before, VkImageLayout after)
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
                                  .format         = img[0]->GetFormat(),
                                  .samples        = img[0]->GetSampleCount(),
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

    Renderpass::Builder& Renderpass::Builder::AddAttachmentColor(const Attachment& attachment)
    {
        mColor.push_back(attachment);
        return *this;
    }

    Renderpass::Builder& Renderpass::Builder::AddAttachmentColorW(core::ManagedImage* img, VkImageLayout during, VkImageLayout after)
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
                                  .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                                  .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                  .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                  .finalLayout    = after,
                              }};
        return AddAttachmentColor(attachment);
    }

    Renderpass::Builder& Renderpass::Builder::AddAttachmentColorW(base::VulkanWindowSwapchain* swapchain, VkImageLayout during, VkImageLayout after)
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
                                  .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                                  .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                  .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                  .finalLayout    = after,
                              }};
        return AddAttachmentColor(attachment);
    }

    Renderpass::Builder& Renderpass::Builder::AddAttachmentsColorW(core::ManagedImage** img, uint32_t count, VkImageLayout during, VkImageLayout after)
    {
        if(after == VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM)
        {
            after = during;
        }
        Attachment attachment{.Source      = AttachmentSource::ManagedImage,
                              .Images      = std::vector<core::ManagedImage*>(count),
                              .Layout      = during,
                              .Description = VkAttachmentDescription{
                                  .flags          = 0,
                                  .format         = img[0]->GetFormat(),
                                  .samples        = img[0]->GetSampleCount(),
                                  .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                                  .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                  .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                  .finalLayout    = after,
                              }};
        memcpy(attachment.Images.data(), img, count * sizeof(core::ManagedImage*));
        return AddAttachmentIn(attachment);
    }

    Renderpass::Builder& Renderpass::Builder::AddAttachmentColorRW(core::ManagedImage* img, VkImageLayout during, VkImageLayout before, VkImageLayout after)
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
                                  .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD,
                                  .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                  .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  .initialLayout  = before,
                                  .finalLayout    = after,
                              }};
        return AddAttachmentColor(attachment);
    }

    Renderpass::Builder& Renderpass::Builder::AddAttachmentColorRW(base::VulkanWindowSwapchain* swapchain, VkImageLayout during, VkImageLayout before, VkImageLayout after)
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
                                  .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD,
                                  .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                  .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  .initialLayout  = before,
                                  .finalLayout    = after,
                              }};
        return AddAttachmentColor(attachment);
    }

    Renderpass::Builder& Renderpass::Builder::AddAttachmentsColorRW(core::ManagedImage** img, uint32_t count, VkImageLayout during, VkImageLayout before, VkImageLayout after)
    {
        if(after == VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM)
        {
            after = during;
        }
        Attachment attachment{.Source      = AttachmentSource::ManagedImage,
                              .Images      = std::vector<core::ManagedImage*>(count),
                              .Layout      = during,
                              .Description = VkAttachmentDescription{
                                  .flags          = 0,
                                  .format         = img[0]->GetFormat(),
                                  .samples        = img[0]->GetSampleCount(),
                                  .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD,
                                  .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                  .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  .initialLayout  = before,
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

    Renderpass::Builder& Renderpass::Builder::SetAttachmentDepthStencil(core::ManagedImage* img, fp32_t depthClearValue, VkImageLayout during, VkImageLayout after)
    {
        if(after == VkImageLayout::VK_IMAGE_LAYOUT_MAX_ENUM)
        {
            after = during;
        }
        return SetAttachmentDepthStencil(Attachment{.Source = AttachmentSource::ManagedImage,
                                                    .Images = {img},
                                                    .Layout = during,
                                                    .Description =
                                                        VkAttachmentDescription{
                                                            .flags          = 0,
                                                            .format         = img->GetFormat(),
                                                            .samples        = img->GetSampleCount(),
                                                            .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                            .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                                            .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                            .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                            .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                                            .finalLayout    = after,
                                                        },
                                                    .ClearValue = VkClearValue{.depthStencil = VkClearDepthStencilValue{.depth = depthClearValue, .stencil = 0u}}});
    }

    Renderpass::Builder& Renderpass::Builder::RemoveAttachmentDepthStencil()
    {
        mDepthStencil.Delete();
        return *this;
    }


    Renderpass::Renderpass(core::Context* context, const Builder& builder) : mContext(context)
    {

        std::vector<VkAttachmentReference>   inputAttachmentRefs;
        std::vector<VkAttachmentReference>   colorAttachmentRefs;
        VkAttachmentReference                depthAttachmentRef = VkAttachmentReference{.attachment = VK_ATTACHMENT_UNUSED};
        uint32_t                             depthIdx           = VK_ATTACHMENT_UNUSED;
        std::vector<VkAttachmentDescription> attachmentDescr;

        {  // Prepare attachment member
            uint32_t attachmentCount = 0;
            attachmentCount += (uint32_t)builder.GetInput().size();
            attachmentCount += (uint32_t)builder.GetColor().size();
            attachmentCount += builder.HasDepthStencil() ? 1u : 0u;
            mAttachments.reserve(attachmentCount);
            mPostRenderStates.reserve(attachmentCount);
            mClearValues.reserve(attachmentCount);
        }

        {  // Input attachments

            for(const Builder::Attachment& builderAttachment : builder.GetInput())
            {
                inputAttachmentRefs.push_back(VkAttachmentReference{.attachment = (uint32_t)attachmentDescr.size(), .layout = builderAttachment.Layout});
                attachmentDescr.push_back(builderAttachment.Description);
                AttachmentInfo info{
                    .Source = builderAttachment.Source, .BareRefs = builderAttachment.BareRefs, .Images = builderAttachment.Images, .Swapchain = builderAttachment.Swapchain};
                mAttachments.push_back(info);
                mPostRenderStates.push_back(builderAttachment.Description.finalLayout);
                mClearValues.push_back(builderAttachment.ClearValue);
            }
        }

        {  // Color attachments
            for(const Builder::Attachment& builderAttachment : builder.GetColor())
            {
                colorAttachmentRefs.push_back(VkAttachmentReference{.attachment = (uint32_t)attachmentDescr.size(), .layout = builderAttachment.Layout});
                attachmentDescr.push_back(builderAttachment.Description);
                AttachmentInfo info{
                    .Source = builderAttachment.Source, .BareRefs = builderAttachment.BareRefs, .Images = builderAttachment.Images, .Swapchain = builderAttachment.Swapchain};
                mAttachments.push_back(info);
                mPostRenderStates.push_back(builderAttachment.Description.finalLayout);
                mClearValues.push_back(builderAttachment.ClearValue);
            }
        }

        if(builder.HasDepthStencil())
        {  // Depth / Stencil attachment
            const Builder::Attachment& builderAttachment = *builder.GetDepthStencil();

            depthIdx           = (uint32_t)colorAttachmentRefs.size();
            depthAttachmentRef = VkAttachmentReference{.attachment = depthIdx, .layout = builderAttachment.Layout};
            attachmentDescr.push_back(builderAttachment.Description);
            AttachmentInfo info{
                .Source = builderAttachment.Source, .BareRefs = builderAttachment.BareRefs, .Images = builderAttachment.Images, .Swapchain = builderAttachment.Swapchain};
            mAttachments.push_back(info);
            mPostRenderStates.push_back(builderAttachment.Description.finalLayout);
            mClearValues.push_back(builderAttachment.ClearValue);
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

            VkPipelineStageFlags beforeStageFlags  = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            VkAccessFlags        beforeAccessFlags = 0;

            VkPipelineStageFlags subpassStageFlags  = 0;
            VkAccessFlags        subpassAccessFlags = 0;

            VkPipelineStageFlags afterStageFlags  = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            VkAccessFlags        afterAccessFlags = 0;

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
                subpassAccessFlags |= VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
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

        ResizeFramebuffers(builder.GetInitialSize());
    }

    Renderpass::~Renderpass()
    {
        for(VkFramebuffer framebuf : mFramebuffers)
        {
            vkDestroyFramebuffer(mContext->VkDevice(), framebuf, nullptr);
        }
        vkDestroyRenderPass(mContext->VkDevice(), mRenderpass, nullptr);
    }

    Renderpass& Renderpass::UpdateBareRef(uint32_t idx, BareAttachment bareRef)
    {
        return UpdateBareRefs(idx, &bareRef, 1u);
    }

    Renderpass& Renderpass::UpdateBareRefs(uint32_t idx, BareAttachment* bareRefs, uint32_t count)
    {
        mAttachments[idx].BareRefs.resize(count);
        memcpy(mAttachments[idx].BareRefs.data(), bareRefs, count);
        return *this;
    }

    void Renderpass::CmdBeginRenderpass(VkCommandBuffer cmdBuffer, uint64_t framebufIdx)
    {
        mCurrentFramebufferIdx = framebufIdx % mFramebuffers.size();

        VkRect2D renderArea{VkOffset2D{}, mExtent};

        VkRenderPassBeginInfo beginInfo{.sType           = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                        .renderPass      = mRenderpass,
                                        .framebuffer     = mFramebuffers[mCurrentFramebufferIdx],
                                        .renderArea      = renderArea,
                                        .clearValueCount = (uint32_t)mClearValues.size(),
                                        .pClearValues    = mClearValues.data()};
        vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
    }

    void Renderpass::CmdEndRenderpass(VkCommandBuffer cmdBuffer, core::ImageLayoutCache& layoutCache)
    {
        Assert(mCurrentFramebufferIdx != FRAMEBUFFERIDX_INVALID);
        for(uint32_t i = 0; i < (uint32_t)mAttachments.size(); i++)
        {
            AttachmentInfo& info   = mAttachments[i];
            VkImageLayout   layout = mPostRenderStates[i];
            VkImage         image  = nullptr;
            switch(info.Source)
            {
                case AttachmentSource::Bare:
                    image = info.BareRefs[mCurrentFramebufferIdx % info.BareRefs.size()].Image;
                    break;
                case AttachmentSource::ManagedImage:
                    image = info.Images[mCurrentFramebufferIdx % info.Images.size()]->GetImage();
                    break;
                case AttachmentSource::Swapchain:
                    image = info.Swapchain->GetSwapchainImages()[mCurrentFramebufferIdx % info.Swapchain->GetSwapchainImages().size()].Image;
                    break;
            }
            if(!!image)
            {
                layoutCache.Set(image, layout);
            }
        }
        mCurrentFramebufferIdx = FRAMEBUFFERIDX_INVALID;
        vkCmdEndRenderPass(cmdBuffer);
    }

    void Renderpass::ResizeFramebuffers(VkExtent2D size)
    {
        for(VkFramebuffer framebuf : mFramebuffers)
        {
            vkDestroyFramebuffer(mContext->VkDevice(), framebuf, nullptr);
        }
        mFramebuffers.clear();
        mExtent = size;

        uint32_t framebufferCount = 0;

        for(uint32_t idx = 0; idx < (uint32_t)mAttachments.size(); idx++)
        {
            AttachmentInfo& info = mAttachments[idx];
            switch(info.Source)
            {
                case AttachmentSource::Bare:
                    framebufferCount = std::max(framebufferCount, (uint32_t)info.BareRefs.size());
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
                    case AttachmentSource::Bare:
                        view = info.BareRefs[fbIdx % (uint32_t)info.BareRefs.size()].View;
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
                                                      .attachmentCount = (uint32_t)attachmentViews.size(),
                                                      .pAttachments    = attachmentViews.data(),
                                                      .width           = mExtent.width,
                                                      .height          = mExtent.height,
                                                      .layers          = 1};
            AssertVkResult(vkCreateFramebuffer(mContext->VkDevice(), &fbufCreateInfo, nullptr, mFramebuffers.data() + fbIdx));
        }
    }

}  // namespace foray::util
