#include "renderattachments.hpp"
#include "../core/imagelayoutcache.hpp"
#include "../core/imageview.hpp"
#include "../core/image.hpp"

namespace foray::util {
    RenderAttachments::Attachment::Attachment() : Source(EAttachmentSource::Undefined) {}
    RenderAttachments::Attachment::Attachment(
        EAttachmentBindpoint bindpoint, std::span<BareAttachment> images, vk::ImageLayout layout, EAttachmentLoadOp loadop, bool store, VkClearValue clearValue)
        : Bindpoint(bindpoint), Source(EAttachmentSource::Bare), BareRefs(images.begin(), images.end()), Layout(layout), LoadOp(loadop), Store(store), ClearValue(clearValue)
    {
    }
    RenderAttachments::Attachment::Attachment(
        EAttachmentBindpoint bindpoint, std::span<core::ImageViewRef*> images, vk::ImageLayout layout, EAttachmentLoadOp loadop, bool store, VkClearValue clearValue)
        : Bindpoint(bindpoint), Source(EAttachmentSource::Image), Images(images.begin(), images.end()), Layout(layout), LoadOp(loadop), Store(store), ClearValue(clearValue)
    {
    }
    RenderAttachments::Attachment::Attachment(
        EAttachmentBindpoint bindpoint, base::VulkanWindowSwapchain* swapchain, vk::ImageLayout layout, EAttachmentLoadOp loadop, bool store, VkClearValue clearValue)
        : Bindpoint(bindpoint), Source(EAttachmentSource::Swapchain), Swapchain(swapchain), Layout(layout), LoadOp(loadop), Store(store), ClearValue(clearValue)
    {
    }

    vk::Image RenderAttachments::Attachment::GetSourceImage(uint32_t resourceIdx) const
    {
        switch(Source)
        {
            case EAttachmentSource::Bare:
                return BareRefs[resourceIdx % (uint32_t)BareRefs.size()].Image;
            case EAttachmentSource::Image:
                return Images[resourceIdx % (uint32_t)Images.size()]->GetImage()->GetImage();
            case EAttachmentSource::Swapchain: {
                std::vector<core::SwapchainImageInfo>& swapImgs = Swapchain->GetSwapchainImages();
                return swapImgs[resourceIdx % swapImgs.size()].Image;
            }
            default:
                Exception::Throw("Unhandled switch path");
        }
    }

    vk::ImageView RenderAttachments::Attachment::GetSourceView(uint32_t resourceIdx) const
    {
        switch(Source)
        {
            case EAttachmentSource::Bare:
                return BareRefs[resourceIdx % (uint32_t)BareRefs.size()].View;
            case EAttachmentSource::Image:
                return Images[resourceIdx % (uint32_t)Images.size()]->GetView();
            case EAttachmentSource::Swapchain: {
                std::vector<core::SwapchainImageInfo>& swapImgs = Swapchain->GetSwapchainImages();
                return swapImgs[resourceIdx % swapImgs.size()].ImageView;
            }
            default:
                Exception::Throw("Unhandled switch path");
        }
    }

    vk::Format RenderAttachments::Attachment::GetSourceFormat() const
    {
        switch(Source)
        {
            case EAttachmentSource::Bare:
                return BareRefs[0].Format;
            case EAttachmentSource::Image:
                return Images[0]->GetImage()->GetFormat();
            case EAttachmentSource::Swapchain:
                return Swapchain->GetSwapchain().image_format;
            default:
                Exception::Throw("Unhandled switch path");
        }
    }

    VkRenderingAttachmentInfo RenderAttachments::Attachment::GetAttachmentInfo(uint32_t resourceIdx) const
    {
        VkAttachmentLoadOp loadOp;
        switch(LoadOp)
        {
            case EAttachmentLoadOp::Clear:
                loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
                break;
            case EAttachmentLoadOp::Load:
                loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
                break;
            case EAttachmentLoadOp::Discard:
                loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                break;
            default:
                Exception::Throw("Unhandled switch path");
        }
        return VkRenderingAttachmentInfo{.sType       = VkStructureType::VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                                         .imageView   = GetSourceView(resourceIdx),
                                         .imageLayout = Layout,
                                         .loadOp      = loadOp,
                                         .storeOp     = Store ? VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE : VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_NONE,
                                         .clearValue  = ClearValue};
    }

    VkImageMemoryBarrier2 RenderAttachments::Attachment::MakeBarrier(uint32_t resourceIdx, core::ImageLayoutCache& layoutCache) const
    {
        vk::Image                          image = GetSourceImage(resourceIdx);
        core::ImageLayoutCache::Barrier2 barrier;
        barrier.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.SrcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
        barrier.NewLayout     = Layout;

        if(LoadOp != EAttachmentLoadOp::Load)
        {
            // We aren't reading any data, setting the source layout manually to undefined
            layoutCache.Set(image, vk::ImageLayout::VK_IMAGE_LAYOUT_UNDEFINED);
        }

        switch(Bindpoint)
        {
            case EAttachmentBindpoint::Color:
                barrier.DstStageMask |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                barrier.DstAccessMask |= LoadOp == EAttachmentLoadOp::Load ? VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case EAttachmentBindpoint::Depth:
                barrier.DstStageMask |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                barrier.DstAccessMask |= LoadOp == EAttachmentLoadOp::Load ? VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT : VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                barrier.SubresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
                break;
            case EAttachmentBindpoint::Stencil:
                barrier.DstStageMask |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                barrier.DstAccessMask |= LoadOp == EAttachmentLoadOp::Load ? VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT : VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                barrier.SubresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
                break;
        }

        if (Source == EAttachmentSource::Image)
        {
            barrier.SubresourceRange = Images[0]->GetSubResource().MakeVkSubresourceRange();
        }

        return layoutCache.MakeBarrier(image, barrier);
    }

    RenderAttachments& RenderAttachments::SetAttachment(uint32_t idx, const Attachment& attachment)
    {
        if(idx >= mAttachments.size())
        {
            mAttachments.resize(idx + 1);
        }
        mAttachments[idx] = attachment;
        return *this;
    }

    RenderAttachments& RenderAttachments::SetAttachmentCleared(uint32_t idx, core::ImageViewRef* image, vk::ImageLayout layout, VkClearColorValue clearValue)
    {
        return SetAttachmentCleared(idx, std::span<core::ImageViewRef*>(&image, 1u), layout, clearValue);
    }

    RenderAttachments& RenderAttachments::SetAttachmentCleared(uint32_t idx, std::span<core::ImageViewRef*> images, vk::ImageLayout layout, VkClearColorValue clearValue)
    {
        return SetAttachment(idx, Attachment(EAttachmentBindpoint::Color, images, layout, EAttachmentLoadOp::Clear, true, VkClearValue{.color = {clearValue}}));
    }

    RenderAttachments& RenderAttachments::SetAttachmentCleared(uint32_t idx, BareAttachment image, vk::ImageLayout layout, VkClearColorValue clearValue)
    {
        return SetAttachmentCleared(idx, std::span<BareAttachment>(&image, 1u), layout, clearValue);
    }

    RenderAttachments& RenderAttachments::SetAttachmentCleared(uint32_t idx, std::span<BareAttachment> images, vk::ImageLayout layout, VkClearColorValue clearValue)
    {
        return SetAttachment(idx, Attachment(EAttachmentBindpoint::Color, images, layout, EAttachmentLoadOp::Clear, true, VkClearValue{.color = {clearValue}}));
    }
    RenderAttachments& RenderAttachments::SetAttachmentCleared(uint32_t idx, base::VulkanWindowSwapchain* swapchain, vk::ImageLayout layout, VkClearColorValue clearValue)
    {
        return SetAttachment(idx, Attachment(EAttachmentBindpoint::Color, swapchain, layout, EAttachmentLoadOp::Clear, true, VkClearValue{.color = {clearValue}}));
    }
    RenderAttachments& RenderAttachments::SetAttachmentDiscarded(uint32_t idx, core::ImageViewRef* image, vk::ImageLayout layout)
    {
        return SetAttachmentDiscarded(idx, std::span<core::ImageViewRef*>(&image, 1u), layout);
    }
    RenderAttachments& RenderAttachments::SetAttachmentDiscarded(uint32_t idx, std::span<core::ImageViewRef*> images, vk::ImageLayout layout)
    {
        return SetAttachment(idx, Attachment(EAttachmentBindpoint::Color, images, layout, EAttachmentLoadOp::Discard, true));
    }
    RenderAttachments& RenderAttachments::SetAttachmentDiscarded(uint32_t idx, base::VulkanWindowSwapchain* swapchain, vk::ImageLayout layout)
    {
        return SetAttachment(idx, Attachment(EAttachmentBindpoint::Color, swapchain, layout, EAttachmentLoadOp::Discard, true));
    }
    RenderAttachments& RenderAttachments::SetAttachmentDiscarded(uint32_t idx, std::span<BareAttachment> images, vk::ImageLayout layout)
    {
        return SetAttachment(idx, Attachment(EAttachmentBindpoint::Color, images, layout, EAttachmentLoadOp::Discard, true));
    }
    RenderAttachments& RenderAttachments::SetAttachmentDiscarded(uint32_t idx, BareAttachment image, vk::ImageLayout layout)
    {
        return SetAttachmentDiscarded(idx, std::span<BareAttachment>(&image, 1u), layout);
    }
    RenderAttachments& RenderAttachments::SetAttachmentLoaded(uint32_t idx, core::ImageViewRef* image, vk::ImageLayout layout)
    {
        return SetAttachmentLoaded(idx, std::span<core::ImageViewRef*>(&image, 1u), layout);
    }
    RenderAttachments& RenderAttachments::SetAttachmentLoaded(uint32_t idx, std::span<BareAttachment> images, vk::ImageLayout layout)
    {
        return SetAttachment(idx, Attachment(EAttachmentBindpoint::Color, images, layout, EAttachmentLoadOp::Load, true));
    }
    RenderAttachments& RenderAttachments::SetAttachmentLoaded(uint32_t idx, std::span<core::ImageViewRef*> images, vk::ImageLayout layout)
    {
        return SetAttachment(idx, Attachment(EAttachmentBindpoint::Color, images, layout, EAttachmentLoadOp::Load, true));
    }
    RenderAttachments& RenderAttachments::SetAttachmentLoaded(uint32_t idx, base::VulkanWindowSwapchain* swapchain, vk::ImageLayout layout)
    {
        return SetAttachment(idx, Attachment(EAttachmentBindpoint::Color, swapchain, layout, EAttachmentLoadOp::Load, true));
    }
    RenderAttachments& RenderAttachments::SetAttachmentLoaded(uint32_t idx, BareAttachment image, vk::ImageLayout layout)
    {
        return SetAttachmentLoaded(idx, std::span<BareAttachment>(&image, 1u), layout);
    }
    RenderAttachments& RenderAttachments::AddAttachment(const Attachment& attachment)
    {
        mAttachments.emplace_back(attachment);
        return *this;
    }

    RenderAttachments& RenderAttachments::AddAttachmentCleared(core::ImageViewRef* image, vk::ImageLayout layout, VkClearColorValue clearValue)
    {
        return AddAttachmentCleared(std::span<core::ImageViewRef*>(&image, 1u), layout, clearValue);
    }

    RenderAttachments& RenderAttachments::AddAttachmentCleared(std::span<core::ImageViewRef*> images, vk::ImageLayout layout, VkClearColorValue clearValue)
    {
        return AddAttachment(Attachment(EAttachmentBindpoint::Color, images, layout, EAttachmentLoadOp::Clear, true, VkClearValue{.color = {clearValue}}));
    }

    RenderAttachments& RenderAttachments::AddAttachmentCleared(BareAttachment image, vk::ImageLayout layout, VkClearColorValue clearValue)
    {
        return AddAttachmentCleared(std::span<BareAttachment>(&image, 1u), layout, clearValue);
    }

    RenderAttachments& RenderAttachments::AddAttachmentCleared(std::span<BareAttachment> images, vk::ImageLayout layout, VkClearColorValue clearValue)
    {
        return AddAttachment(Attachment(EAttachmentBindpoint::Color, images, layout, EAttachmentLoadOp::Clear, true, VkClearValue{.color = {clearValue}}));
    }
    RenderAttachments& RenderAttachments::AddAttachmentCleared(base::VulkanWindowSwapchain* swapchain, vk::ImageLayout layout, VkClearColorValue clearValue)
    {
        return AddAttachment(Attachment(EAttachmentBindpoint::Color, swapchain, layout, EAttachmentLoadOp::Clear, true, VkClearValue{.color = {clearValue}}));
    }
    RenderAttachments& RenderAttachments::AddAttachmentDiscarded(core::ImageViewRef* image, vk::ImageLayout layout)
    {
        return AddAttachmentDiscarded(std::span<core::ImageViewRef*>(&image, 1u), layout);
    }
    RenderAttachments& RenderAttachments::AddAttachmentDiscarded(std::span<core::ImageViewRef*> images, vk::ImageLayout layout)
    {
        return AddAttachment(Attachment(EAttachmentBindpoint::Color, images, layout, EAttachmentLoadOp::Discard, true));
    }
    RenderAttachments& RenderAttachments::AddAttachmentDiscarded(base::VulkanWindowSwapchain* swapchain, vk::ImageLayout layout)
    {
        return AddAttachment(Attachment(EAttachmentBindpoint::Color, swapchain, layout, EAttachmentLoadOp::Discard, true));
    }
    RenderAttachments& RenderAttachments::AddAttachmentDiscarded(std::span<BareAttachment> images, vk::ImageLayout layout)
    {
        return AddAttachment(Attachment(EAttachmentBindpoint::Color, images, layout, EAttachmentLoadOp::Discard, true));
    }
    RenderAttachments& RenderAttachments::AddAttachmentDiscarded(BareAttachment image, vk::ImageLayout layout)
    {
        return AddAttachmentDiscarded(std::span<BareAttachment>(&image, 1u), layout);
    }
    RenderAttachments& RenderAttachments::AddAttachmentLoaded(core::ImageViewRef* image, vk::ImageLayout layout)
    {
        return AddAttachmentLoaded(std::span<core::ImageViewRef*>(&image, 1u), layout);
    }
    RenderAttachments& RenderAttachments::AddAttachmentLoaded(std::span<BareAttachment> images, vk::ImageLayout layout)
    {
        return AddAttachment(Attachment(EAttachmentBindpoint::Color, images, layout, EAttachmentLoadOp::Load, true));
    }
    RenderAttachments& RenderAttachments::AddAttachmentLoaded(std::span<core::ImageViewRef*> images, vk::ImageLayout layout)
    {
        return AddAttachment(Attachment(EAttachmentBindpoint::Color, images, layout, EAttachmentLoadOp::Load, true));
    }
    RenderAttachments& RenderAttachments::AddAttachmentLoaded(base::VulkanWindowSwapchain* swapchain, vk::ImageLayout layout)
    {
        return AddAttachment(Attachment(EAttachmentBindpoint::Color, swapchain, layout, EAttachmentLoadOp::Load, true));
    }
    RenderAttachments& RenderAttachments::AddAttachmentLoaded(BareAttachment image, vk::ImageLayout layout)
    {
        return AddAttachmentLoaded(std::span<BareAttachment>(&image, 1u), layout);
    }
    RenderAttachments& RenderAttachments::SetDepthAttachmentCleared(core::ImageViewRef* image, vk::ImageLayout layout, VkClearDepthStencilValue clearValue, bool store)
    {
        mDepthAttachment.New(Attachment(EAttachmentBindpoint::Depth, std::span<core::ImageViewRef*>(&image, 1u), layout, EAttachmentLoadOp::Clear, store,
                                        VkClearValue{.depthStencil = {clearValue}}));
        return *this;
    }
    RenderAttachments& RenderAttachments::SetDepthAttachmentCleared(BareAttachment image, vk::ImageLayout layout, VkClearDepthStencilValue clearValue, bool store)
    {
        mDepthAttachment.New(
            Attachment(EAttachmentBindpoint::Depth, std::span<BareAttachment>(&image, 1u), layout, EAttachmentLoadOp::Clear, store, VkClearValue{.depthStencil = {clearValue}}));
        return *this;
    }
    RenderAttachments& RenderAttachments::SetDepthAttachmentLoaded(core::ImageViewRef* image, vk::ImageLayout layout, bool store)
    {
        mDepthAttachment.New(Attachment(EAttachmentBindpoint::Depth, std::span<core::ImageViewRef*>(&image, 1u), layout, EAttachmentLoadOp::Load, store));
        return *this;
    }
    RenderAttachments& RenderAttachments::SetDepthAttachmentLoaded(BareAttachment image, vk::ImageLayout layout, bool store)
    {
        mDepthAttachment.New(Attachment(EAttachmentBindpoint::Depth, std::span<BareAttachment>(&image, 1u), layout, EAttachmentLoadOp::Load, store));
        return *this;
    }
    RenderAttachments& RenderAttachments::SetStencilAttachmentCleared(core::ImageViewRef* image, vk::ImageLayout layout, VkClearDepthStencilValue clearValue, bool store)
    {
        mStencilAttachment.New(Attachment(EAttachmentBindpoint::Stencil, std::span<core::ImageViewRef*>(&image, 1u), layout, EAttachmentLoadOp::Clear, store,
                                          VkClearValue{.depthStencil = {clearValue}}));
        return *this;
    }
    RenderAttachments& RenderAttachments::SetStencilAttachmentCleared(BareAttachment image, vk::ImageLayout layout, VkClearDepthStencilValue clearValue, bool store)
    {
        mStencilAttachment.New(
            Attachment(EAttachmentBindpoint::Stencil, std::span<BareAttachment>(&image, 1u), layout, EAttachmentLoadOp::Clear, store, VkClearValue{.depthStencil = {clearValue}}));
        return *this;
    }
    RenderAttachments& RenderAttachments::SetStencilAttachmentLoaded(core::ImageViewRef* image, vk::ImageLayout layout, bool store)
    {
        mStencilAttachment.New(Attachment(EAttachmentBindpoint::Stencil, std::span<core::ImageViewRef*>(&image, 1u), layout, EAttachmentLoadOp::Load, store));
        return *this;
    }
    RenderAttachments& RenderAttachments::SetStencilAttachmentLoaded(BareAttachment image, vk::ImageLayout layout, bool store)
    {
        mStencilAttachment.New(Attachment(EAttachmentBindpoint::Stencil, std::span<BareAttachment>(&image, 1u), layout, EAttachmentLoadOp::Load, store));
        return *this;
    }
    VkPipelineRenderingCreateInfo RenderAttachments::MakePipelineRenderingCi()
    {
        mAttachmentFormats.clear();
        mAttachmentFormats.reserve(mAttachments.size());
        for(const Attachment& attachment : mAttachments)
        {
            mAttachmentFormats.push_back(attachment.GetSourceFormat());
        }
        return VkPipelineRenderingCreateInfo{
            .sType                   = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .viewMask                = mViewMask,
            .colorAttachmentCount    = (uint32_t)mAttachmentFormats.size(),
            .pColorAttachmentFormats = mAttachmentFormats.data(),
            .depthAttachmentFormat   = mDepthAttachment.Exists() ? mDepthAttachment->GetSourceFormat() : vk::Format::VK_FORMAT_UNDEFINED,
            .stencilAttachmentFormat = mStencilAttachment.Exists() ? mStencilAttachment->GetSourceFormat() : vk::Format::VK_FORMAT_UNDEFINED
        };
    }
    void RenderAttachments::CmdBeginRendering(VkCommandBuffer cmdBuffer, VkExtent2D extent, core::ImageLayoutCache& layoutCache, uint32_t resourceIdx) const
    {
        std::vector<VkRenderingAttachmentInfo> attachmentInfos;
        std::vector<VkImageMemoryBarrier2>     imageBarriers;
        Local<VkRenderingAttachmentInfo>       depthAttachmentInfo;
        Local<VkRenderingAttachmentInfo>       stencilAttachmentInfo;

        // Process color attachments
        for(const Attachment& attachment : mAttachments)
        {
            Assert(attachment.Source != EAttachmentSource::Undefined);
            attachmentInfos.emplace_back(attachment.GetAttachmentInfo(resourceIdx));
            imageBarriers.emplace_back(attachment.MakeBarrier(resourceIdx, layoutCache));
        }

        // Process depth attachment
        if(mDepthAttachment.Exists())
        {
            Assert(mDepthAttachment->Source != EAttachmentSource::Undefined);
            depthAttachmentInfo.New(mDepthAttachment->GetAttachmentInfo(resourceIdx));
            imageBarriers.emplace_back(mDepthAttachment->MakeBarrier(resourceIdx, layoutCache));
        }

        // Process stencil attachment
        if(mStencilAttachment.Exists())
        {
            Assert(mStencilAttachment->Source != EAttachmentSource::Undefined);
            stencilAttachmentInfo.New(mStencilAttachment->GetAttachmentInfo(resourceIdx));
            imageBarriers.emplace_back(mStencilAttachment->MakeBarrier(resourceIdx, layoutCache));
        }

        {  // Add pipeline barrier
            VkDependencyInfo depInfo{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                     .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
                                     .imageMemoryBarrierCount = (uint32_t)imageBarriers.size(),
                                     .pImageMemoryBarriers    = imageBarriers.data()};
            vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
        }

        VkRenderingInfo renderingInfo{
            .sType                = VkStructureType::VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext                = mPNext,
            .flags                = mFlags,
            .renderArea           = VkRect2D{{}, extent},
            .layerCount           = mLayerCount,
            .viewMask             = mViewMask,
            .colorAttachmentCount = (uint32_t)attachmentInfos.size(),
            .pColorAttachments    = attachmentInfos.data(),
            .pDepthAttachment     = depthAttachmentInfo.GetNullable(),
            .pStencilAttachment   = stencilAttachmentInfo.GetNullable(),
        };

        vkCmdBeginRendering(cmdBuffer, &renderingInfo);
    }
}  // namespace foray::util