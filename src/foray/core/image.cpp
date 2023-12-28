#include "image.hpp"
#include "commandbuffer.hpp"
#include "context.hpp"
#include "imageview.hpp"
#include "managedbuffer.hpp"

namespace foray::core {
    VkExtent2D IImage::GetExtent2D() const
    {
        vk::Extent3D extent = GetExtent();
        return VkExtent2D{extent.width, extent.height};
    }

    Image::Image(core::Context* context, const CreateInfo& createInfo) : mContext(context), mCreateInfo(createInfo)
    {
        mCreateInfo.Validate();
        mCreateInfo.ValidateFormatSupport(mContext->VkPhysicalDevice());

        VkImageCreateInfo       vulkanCreateInfo = mCreateInfo.GetVulkanCreateInfo();
        VmaAllocationCreateInfo allocCreateInfo  = mCreateInfo.GetVmaCreateInfo();
        AssertVkResult(vmaCreateImage(mContext->Allocator, &vulkanCreateInfo, &allocCreateInfo, &mImage, &mAllocation, &mAllocationInfo));

        SetObjectName(mContext, mImage, mCreateInfo.GetName());
        vmaSetAllocationName(mContext->Allocator, mAllocation, GetName().data());
    }

    Image::~Image()
    {
        for(auto& kvp : mViews)
        {
            RefCountedView& view = kvp.second;
            for(ImageViewRef* viewRef : view.Refs)
            {
                viewRef->mImage = nullptr;
                viewRef->mView  = nullptr;
            }
            if(!!view.View)
            {
                vkDestroyImageView(mContext->VkDevice(), view.View, nullptr);
            }
        }
        vmaDestroyImage(mContext->Allocator, mImage, mAllocation);
        mImage      = nullptr;
        mAllocation = nullptr;
    }

    void Image::GetImageView(ImageViewRef& viewRef)
    {
        const ImageSubResource& subResource = viewRef.GetSubResource();
        uint64_t                viewHash    = subResource.CalculateHash();
        auto                    iter        = mViews.find(viewHash);
        if(iter != mViews.end())
        {
            viewRef.mView = iter->second.View;
            iter->second.Refs.push_back(&viewRef);
        }
        else
        {
            VkImageViewCreateInfo viewCi{.sType            = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                         .pNext            = nullptr,
                                         .flags            = 0,
                                         .image            = mImage,
                                         .viewType         = subResource.GetViewType(),
                                         .format           = mCreateInfo.GetFormat(),
                                         .components       = VkComponentMapping{VkComponentSwizzle::VK_COMPONENT_SWIZZLE_R, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_G,
                                                                          VkComponentSwizzle::VK_COMPONENT_SWIZZLE_B, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_A},
                                         .subresourceRange = VkImageSubresourceRange{.aspectMask     = subResource.GetAspectFlags(),
                                                                                     .baseMipLevel   = subResource.GetMipLevels().Base,
                                                                                     .levelCount     = subResource.GetMipLevels().Count,
                                                                                     .baseArrayLayer = subResource.GetArrayLayers().Base,
                                                                                     .layerCount     = subResource.GetArrayLayers().Count}};

            AssertVkResult(vkCreateImageView(mContext->VkDevice(), &viewCi, nullptr, &viewRef.mView));
            mViews.emplace(viewHash, RefCountedView(viewRef.mView, viewRef));
        }
    }

    void Image::ReturnImageView(ImageViewRef& view)
    {
        Assert(view.mImage = this);
        if(view.Exists())
        {
            uint64_t viewHash = view.GetSubResource().CalculateHash();

            auto iter = mViews.find(viewHash);
            Assert(iter != mViews.end());
            const auto iter2 = std::find(iter->second.Refs.cbegin(), iter->second.Refs.cend(), &view);
            Assert(iter->second.View == view.GetView() && iter2 != iter->second.Refs.cend());
            iter->second.Refs.erase(iter2);
            if(iter->second.Refs.empty())
            {
                vkDestroyImageView(mContext->VkDevice(), view.mView, nullptr);
            }
        }
        view.mView  = nullptr;
        view.mImage = nullptr;
    }
    Image::RefCountedView::RefCountedView() : View(nullptr) {}
    Image::RefCountedView::RefCountedView(vk::ImageView view, ImageViewRef& initialRef) : View(view), Refs({&initialRef}) {}
}  // namespace foray::core

namespace foray {
    Local<core::Image>::Local(core::Context* context, const core::Image::CreateInfo& createInfo) : LocalBase<core::Image>(context, createInfo) {}
    void Local<core::Image>::New(core::Context* context, const core::Image::CreateInfo& createInfo)
    {
        LocalBase<core::Image>::New(context, createInfo);
    }
    void Local<core::Image>::Resize(VkExtent2D extent)
    {
        Assert(Exists());
        core::Context*          context = Get()->GetContext();
        core::Image::CreateInfo ci      = Get()->GetCreateInfo();
        ci.SetExtent(extent);
        New(context, ci);
    }
    void Local<core::Image>::Resize(vk::Extent3D extent)
    {
        Assert(Exists());
        core::Context*          context = Get()->GetContext();
        core::Image::CreateInfo ci      = Get()->GetCreateInfo();
        ci.SetExtent(extent);
        New(context, ci);
    }
}  // namespace foray
