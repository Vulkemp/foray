#include "imageview.hpp"
#include "../util/hash.hpp"
#include "image.hpp"
#include <unordered_map>

namespace foray::core {
    vk::Format IImageView::GetFormat() const
    {
        return GetImage()->GetFormat();
    }

    vk::Extent3D IImageView::GetExtent() const
    {
        return GetImage()->GetExtent();
    }

    VkExtent2D IImageView::GetExtent2D() const
    {
        vk::Extent3D extent = GetExtent();
        return VkExtent2D{extent.width, extent.height};
    }


#pragma region ImageSubResource
    uint64_t ImageSubResource::CalculateHash() const
    {
        uint64_t hash = 0;
        util::AccumulateHash(hash, mViewType);
        util::AccumulateHash(hash, mAspectFlags);
        util::AccumulateHash(hash, mMipLevels.Base);
        util::AccumulateHash(hash, mMipLevels.Count);
        util::AccumulateHash(hash, mArrayLayers.Base);
        util::AccumulateHash(hash, mArrayLayers.Count);
        return hash;
    }

    ImageSubResource::ImageSubResource(Image* image)
    {
        Assert(!!image);
        const Image::CreateInfo& imageCi = image->GetCreateInfo();
        switch(imageCi.GetType())
        {
            case VkImageType::VK_IMAGE_TYPE_1D:
                SetViewType(vk::ImageViewType::e1D);
                break;
            case VkImageType::VK_IMAGE_TYPE_2D:
                SetViewType(vk::ImageViewType::e2D);
                break;
            case VkImageType::VK_IMAGE_TYPE_3D:
                SetViewType(vk::ImageViewType::e3D);
                break;
            default:
                Exception::Throw("Unhandled Switch Value");
        }
        SetMipLevels(Range{0, imageCi.GetMipLevelCount() - 1});
        SetArrayLayers(Range{0, imageCi.GetArrayLayerCount() - 1});
    }

    ImageSubResource::ImageSubResource(Image* image, vk::ImageViewType type, vk::ImageAspectFlags aspectFlags)
    {
        Assert(!!image);
        const Image::CreateInfo& imageCi = image->GetCreateInfo();
        SetViewType(type);
        SetAspectFlags(type);
        SetMipLevels(Range{0, imageCi.GetMipLevelCount() - 1});
        SetArrayLayers(Range{0, imageCi.GetArrayLayerCount() - 1});
    }

    vk::ImageSubresourceRange ImageSubResource::MakeVkSubresourceRange() const
    {
        return vk::ImageSubresourceRange(GetAspectFlags(), GetMipLevels().Base, GetMipLevels().Count, GetArrayLayers().Base, GetArrayLayers().Count);
    }

#pragma endregion
#pragma region ImageViewRef

    ImageViewRef::ImageViewRef(Image* image) : mImage(image), mSubResource(image)
    {
        image->GetImageView(*this);
    }

    ImageViewRef::~ImageViewRef()
    {
        if(Exists())
        {
            mImage->ReturnImageView(*this);
        }
    }

    void ImageViewRef::Destroy()
    {
        if(Exists())
        {
            mImage->ReturnImageView(*this);
        }
    }

    bool ImageViewRef::Exists() const
    {
        return !!mView;
    }

    vk::ImageView ImageViewRef::GetView() const
    {
        return mView;
    }

    IImage* ImageViewRef::GetImage()
    {
        return mImage;
    }

    IImage* ImageViewRef::GetImage() const
    {
        return mImage;
    }

    const ImageSubResource& ImageViewRef::GetSubResource() const
    {
        return mSubResource;
    }

    ImageViewRef::ImageViewRef(Image* image, ImageSubResource viewResource) : mImage(image), mSubResource(viewResource)
    {
        image->GetImageView(*this);
    }

#pragma endregion

}  // namespace foray::core
