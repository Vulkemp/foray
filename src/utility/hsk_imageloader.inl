#pragma once
#include "../hsk_env.hpp"
#include "hsk_imageloader.hpp"
#include "hsk_imageloader_exr.inl"
#include "hsk_imageloader_stb.inl"
#include "../memory/hsk_managedimage.hpp"

using namespace std::filesystem;

namespace hsk {

    template <VkFormat FORMAT>
    bool ImageLoader<FORMAT>::sFormatSupported(const VkContext* context)
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
        Cleanup();

        mInfo.Utf8Path = utf8path;

        path fspath = FromUtf8Path(utf8path);
        if(fspath.has_extension())
        {
            mInfo.Extension = ToUtf8Path(fspath.extension());
        }
        if(fspath.has_filename())
        {
            mInfo.Name = ToUtf8Path(fspath.filename());
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
    void ImageLoader<FORMAT>::Cleanup()
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
    inline void ImageLoader<FORMAT>::InitManagedImage(const VkContext* context, ManagedImage* image, ManagedImage::CreateInfo& ci, VkImageLayout afterwrite) const
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
    inline void ImageLoader<FORMAT>::InitManagedImage(const VkContext* context, CommandBuffer& cmdBuffer, ManagedImage* image, ManagedImage::CreateInfo& ci, VkImageLayout afterwrite) const
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
    inline void ImageLoader<FORMAT>::UpdateManagedImageCI(ManagedImage::CreateInfo& ci) const
    {
        ci.ImageCI.format        = FORMAT;
        ci.ImageCI.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        ci.ImageCI.usage         = ci.ImageCI.usage | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        ci.ImageCI.extent        = VkExtent3D{.width = mInfo.Extent.width, .height = mInfo.Extent.height, .depth = 1};
        ci.ImageCI.imageType     = VkImageType::VK_IMAGE_TYPE_2D;
    }

    template <VkFormat FORMAT>
    inline void ImageLoader<FORMAT>::WriteManagedImageData(ManagedImage* image,  VkImageLayout afterwrite) const
    {
        image->WriteDeviceLocalData(mRawData.data(), mRawData.size(), afterwrite);
    }

    template <VkFormat FORMAT>
    inline void ImageLoader<FORMAT>::WriteManagedImageData(CommandBuffer& cmdBuffer, ManagedImage* image,  VkImageLayout afterwrite) const
    {
        image->WriteDeviceLocalData(cmdBuffer, mRawData.data(), mRawData.size(), afterwrite);
    }


}  // namespace hsk