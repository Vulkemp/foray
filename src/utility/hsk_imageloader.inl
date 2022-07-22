#pragma once
#include "../hsk_env.hpp"
#include "hsk_imageloader.hpp"
#include "hsk_imageloader_exr.inl"

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
        // Inplace new requires explicit destructor call
        mInfo.~ImageInfo();
        new(&mInfo) ImageInfo();

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
            return false;
        }
        // else if(mInfo.Extension == ".hdr")
        // {
        //     return PopulateImageInfo_StbHdr();
        // }
        // else
        // {
        //     return PopulateImageInfo_StbLdr();
        // }
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
            return false;
        }
        // else if(mInfo.Extension == ".hdr")
        // {
        //     return Load_StbHdr();
        // }
        // else
        // {
        //     return Load_StbLdr();
        // }
    }

    template <VkFormat FORMAT>
    void ImageLoader<FORMAT>::Cleanup()
    {
        if(mCustomLoaderInfoDeleter && mCustomLoaderInfo)
        {
            mCustomLoaderInfoDeleter(mCustomLoaderInfo);
        }
        mRawData.clear();
        mInfo.~ImageInfo();
        new(&mInfo) ImageInfo();
    }

}  // namespace hsk