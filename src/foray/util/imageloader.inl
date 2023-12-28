#pragma once
#include "../core/image.hpp"
#include "../osi/path.hpp"
#include "imageloader.hpp"
#include "imageloader_exr.inl"
#include "imageloader_stb.inl"

using namespace std::filesystem;

namespace foray::util {

    template <vk::Format FORMAT>
    bool ImageLoader<FORMAT>::sFormatSupported(core::Context* context)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(context->VkPhysicalDevice(), FORMAT, &properties);
        if((properties.linearTilingFeatures & VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT & VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_TRANSFER_DST_BIT) > 0)
        {
            return true;
        }
        return false;
    }

    template <vk::Format FORMAT>
    bool ImageLoader<FORMAT>::Init(const osi::Utf8Path& utf8path)
    {
        // Reset all members
        Destroy();

        mInfo.Utf8Path = utf8path;

        path fspath = utf8path;
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

    template <vk::Format FORMAT>
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

    template <vk::Format FORMAT>
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

    template <vk::Format FORMAT>
    inline void ImageLoader<FORMAT>::UpdateManagedImageCI(core::Image::CreateInfo& ci) const
    {
        ci.SetFormat(FORMAT).AddUsageFlagsBits(vk::ImageUsageFlagBits::eTransferDst).SetExtent(mInfo.Extent);
    }

    template <vk::Format FORMAT>
    inline void ImageLoader<FORMAT>::WriteManagedImageData(core::Image* image, vk::ImageLayout afterwrite) const
    {
        // TODO Joseph ImageUploader Support
        // image->UploadToDeviceSynchronized(mRawData.data(), mRawData.size(), afterwrite);
    }

    template <vk::Format FORMAT>
    inline void ImageLoader<FORMAT>::WriteManagedImageData(core::HostSyncCommandBuffer& cmdBuffer, core::Image* image, vk::ImageLayout afterwrite) const
    {
        // TODO Joseph ImageUploader Support
        // image->WriteDeviceLocalData(cmdBuffer, mRawData.data(), mRawData.size(), afterwrite);
    }


}  // namespace foray::util