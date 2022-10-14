#pragma once
#include "../osi/foray_env.hpp"
#include "foray_imageloader.hpp"
#include "foray_imageloader_exr.inl"
#include "foray_imageloader_stb.inl"
#include "../core/foray_managedimage.hpp"

using namespace std::filesystem;

namespace foray::util {

    template <VkFormat FORMAT>
    bool ImageLoader<FORMAT>::sFormatSupported(const core::VkContext* context)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(context->PhysicalDevice.physical_device, FORMAT, &properties);
        if((properties.linearTilingFeatures & VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT & VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_TRANSFER_DST_BIT) > 0)
        {
            return true;
        }
        return false;
    }

    template <VkFormat FORMAT>
    bool ImageLoader<FORMAT>::Init(std::string_view utf8path)
    {
        // Reset all members
        Destroy();

        mInfo.Utf8Path = utf8path;

        path fspath = osi::FromUtf8Path(utf8path);
        if(fspath.has_extension())
        {
            mInfo.Extension = osi::ToUtf8Path(fspath.extension());
        }
        if(fspath.has_filename())
        {
            mInfo.Name = osi::ToUtf8Path(fspath.filename());
        }

        if(!exists(fspath))
        {
            return false;
        }

        if(mInfo.Extension == ".exr")
        {
            return PopulateImageInfo_TinyExr();
        }
        else
        {
            return PopulateImageInfo_Stb();
        }
    }

    template <VkFormat FORMAT>
    bool ImageLoader<FORMAT>::Load()
    {
        if(!mInfo.Valid)
        {
            logger()->warn("Image Loader: Invalid Image Info!");
            return false;
        }

        if(mInfo.Extension == ".exr")
        {
            return Load_TinyExr();
        }
        else
        {
            return Load_Stb();
        }
    }

    template <VkFormat FORMAT>
    void ImageLoader<FORMAT>::Destroy()
    {
        if(mCustomLoaderInfoDeleter && mCustomLoaderInfo)
        {
            mCustomLoaderInfoDeleter(mCustomLoaderInfo);
        }
        mCustomLoaderInfo        = nullptr;
        mCustomLoaderInfoDeleter = nullptr;
        mRawData.clear();

        // Inplace new requires explicit destructor call
        mInfo.~ImageInfo();
        new(&mInfo) ImageInfo();
    }

    template <VkFormat FORMAT>
    inline void ImageLoader<FORMAT>::InitManagedImage(const core::VkContext* context, core::ManagedImage* image, core::ManagedImage::CreateInfo& ci, VkImageLayout afterwrite) const
    {
        if(!mInfo.Valid || !mRawData.size())
        {
            return;
        }

        UpdateManagedImageCI(ci);

        image->Create(context, ci);
        WriteManagedImageData(image, afterwrite);
    }

    template <VkFormat FORMAT>
    inline void ImageLoader<FORMAT>::InitManagedImage(const core::VkContext* context, core::HostCommandBuffer& cmdBuffer, core::ManagedImage* image, core::ManagedImage::CreateInfo& ci, VkImageLayout afterwrite) const
    {
        if(!mInfo.Valid || !mRawData.size())
        {
            return;
        }

        UpdateManagedImageCI(ci);

        image->Create(context, ci);
        WriteManagedImageData(cmdBuffer, image, afterwrite);
    }

    template <VkFormat FORMAT>
    inline void ImageLoader<FORMAT>::UpdateManagedImageCI(core::ManagedImage::CreateInfo& ci) const
    {
        ci.ImageCI.format        = FORMAT;
        ci.ImageCI.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        ci.ImageCI.usage         = ci.ImageCI.usage | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        ci.ImageCI.extent        = VkExtent3D{.width = mInfo.Extent.width, .height = mInfo.Extent.height, .depth = 1};
        ci.ImageCI.imageType     = VkImageType::VK_IMAGE_TYPE_2D;
    }

    template <VkFormat FORMAT>
    inline void ImageLoader<FORMAT>::WriteManagedImageData(core::ManagedImage* image,  VkImageLayout afterwrite) const
    {
        image->WriteDeviceLocalData(mRawData.data(), mRawData.size(), afterwrite);
    }

    template <VkFormat FORMAT>
    inline void ImageLoader<FORMAT>::WriteManagedImageData(core::HostCommandBuffer& cmdBuffer, core::ManagedImage* image,  VkImageLayout afterwrite) const
    {
        image->WriteDeviceLocalData(cmdBuffer, mRawData.data(), mRawData.size(), afterwrite);
    }


}  // namespace foray