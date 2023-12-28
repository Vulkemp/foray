#include "image.hpp"

namespace foray::core
{
    VkImageType Image::CreateInfo::GetType() const
    {
        if(mExtent.height > 1u)
        {
            if(mExtent.depth > 1u)
            {
                return VkImageType::VK_IMAGE_TYPE_3D;
            }
            else
            {
                return VkImageType::VK_IMAGE_TYPE_2D;
            }
        }
        else
        {
            return VkImageType::VK_IMAGE_TYPE_1D;
        }
    }

    Image::CreateInfo& Image::CreateInfo::SetExtent(uint32_t length)
    {
        mExtent = vk::Extent3D{length, 1u, 1u};
        return *this;
    }
    Image::CreateInfo& Image::CreateInfo::SetExtent(uint32_t width, uint32_t height)
    {
        mExtent = vk::Extent3D{width, height, 1u};
        return *this;
    }
    Image::CreateInfo& Image::CreateInfo::SetExtent(VkExtent2D extent)
    {
        mExtent = vk::Extent3D{extent.width, extent.height, 1u};
        return *this;
    }
    Image::CreateInfo& Image::CreateInfo::SetExtent(uint32_t width, uint32_t height, uint32_t depth)
    {
        mExtent = vk::Extent3D{width, height, depth};
        return *this;
    }

    VkExtent2D Image::CreateInfo::GetExtent2D() const
    {
        return VkExtent2D{mExtent.width, mExtent.height};
    }

    uint32_t Image::CreateInfo::GetExtent1D() const
    {
        return mExtent.width;
    }

    VmaAllocationCreateInfo Image::CreateInfo::GetVmaCreateInfo() const
    {
        return VmaAllocationCreateInfo{.flags          = mAllocationCreateFlags,
                                       .usage          = mMemoryUsage,
                                       .requiredFlags  = mRequiredMemoryPropertyFlags,
                                       .preferredFlags = mPreferredMemoryPropertyFlags,
                                       .memoryTypeBits = mMemoryTypeBits,
                                       .pool           = mMemoryPool,
                                       .priority       = mMemoryPriority};
    }

    void Image::CreateInfo::Validate() const
    {
        Assert((mExtent.width > 0u && mExtent.height > 0u && mExtent.depth > 0u), "Image::CreateInfo .mExtent values must not be 0");
        Assert(mMipLevelCount > 0u, "Image::CreateInfo .mMipLevelCount must not be 0");
        Assert(mArrayLayerCount > 0u, "Image::CreateInfo .mArrayLayerCount must not be 0");
    }

    bool Image::CreateInfo::CheckFormatSupport(VkPhysicalDevice physicalDevice) const
    {
        const vk::PhysicalDeviceImageFormatInfo2 formatInfo{.sType  = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
                                                          .pNext  = nullptr,
                                                          .format = mFormat,
                                                          .type   = GetType(),
                                                          .tiling = GetTiling(),
                                                          .usage  = GetUsageFlags(),
                                                          .flags  = GetFlags()};
        VkImageFormatProperties2               formatProperties2{};
        if(vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, &formatInfo, &formatProperties2) != VK_SUCCESS)
        {
            return false;
        }
        const VkImageFormatProperties& formatProperties = formatProperties2.imageFormatProperties;

        return (mExtent.width <= formatProperties.maxExtent.width) && (mExtent.height <= formatProperties.maxExtent.height) && (mExtent.depth <= formatProperties.maxExtent.depth)
               && (mMipLevelCount <= formatProperties.maxMipLevels) && (mArrayLayerCount <= formatProperties.maxArrayLayers);
    }

    void Image::CreateInfo::ValidateFormatSupport(VkPhysicalDevice physicalDevice) const
    {
        const VkPhysicalDeviceImageFormatInfo2 formatInfo{.sType  = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
                                                          .pNext  = nullptr,
                                                          .format = mFormat,
                                                          .type   = GetType(),
                                                          .tiling = GetTiling(),
                                                          .usage  = GetUsageFlags(),
                                                          .flags  = GetFlags()};
        VkImageFormatProperties2               formatProperties2{};
        Assert(vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, &formatInfo, &formatProperties2) == VK_SUCCESS, "Image::CreateInfo Format unsupported");
        const VkImageFormatProperties& formatProperties = formatProperties2.imageFormatProperties;
        Assert(mExtent.width <= formatProperties.maxExtent.width, "Image::CreateInfo Max extent width exceeded");
        Assert(mExtent.height <= formatProperties.maxExtent.height, "Image::CreateInfo Max extent height exceeded");
        Assert(mExtent.depth <= formatProperties.maxExtent.depth, "Image::CreateInfo Max extent depth exceeded");
        Assert(mMipLevelCount <= formatProperties.maxMipLevels, "Image::CreateInfo Max mip level count exceeded");
        Assert(mArrayLayerCount <= formatProperties.maxArrayLayers, "Image::CreateInfo Max array layer count exceeded");
    }

    Image::CreateInfo Image::CreateInfo::PresetCompute(vk::Format format, VkExtent2D extent, bool addBlitFlags, bool addSampledFlag)
    {
        return PresetCompute(format, vk::Extent3D{extent.width, extent.height, 1u}, addBlitFlags, addSampledFlag);
    }

    Image::CreateInfo Image::CreateInfo::PresetCompute(vk::Format format, vk::Extent3D extent, bool addBlitFlags, bool addSampledFlag)
    {
        CreateInfo ci;
        ci.SetFormat(format).SetExtent(extent);
        ci.AddUsageFlagsBits(vk::ImageUsageFlagBits::eStorage);
        if(addBlitFlags)
        {
            ci.AddUsageFlagsBits(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc);
        }
        if(addSampledFlag)
        {
            ci.AddUsageFlagsBits(vk::ImageUsageFlagBits::eSampled);
        }
        return ci;
    }

    Image::CreateInfo Image::CreateInfo::PresetTexture(vk::Format format, VkExtent2D extent, uint32_t mipLevelCount)
    {
        return PresetTexture(format, vk::Extent3D{extent.width, extent.height, 1u}, mipLevelCount);
    }

    Image::CreateInfo Image::CreateInfo::PresetTexture(vk::Format format, vk::Extent3D extent, uint32_t mipLevelCount)
    {
        CreateInfo ci;
        ci.SetFormat(format).SetExtent(extent).SetMipLevelCount(mipLevelCount);
        ci.AddUsageFlagsBits(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
        if(mipLevelCount > 1u)
        {
            ci.AddUsageFlagsBits(vk::ImageUsageFlagBits::eTransferSrc);
        }
        return ci;
    }

    Image::CreateInfo Image::CreateInfo::PresetAttachment(vk::Format format, VkExtent2D extent, bool addBlitFlags, bool addSampledFlag, bool addStorageFlag)
    {
        return PresetAttachment(format, vk::Extent3D{extent.width, extent.height, 1u}, addBlitFlags, addSampledFlag, addStorageFlag);
    }

    Image::CreateInfo Image::CreateInfo::PresetAttachment(vk::Format format, vk::Extent3D extent, bool addBlitFlags, bool addSampledFlag, bool addStorageFlag)
    {
        CreateInfo ci;
        ci.SetFormat(format).SetExtent(extent);
        ci.AddUsageFlagsBits(vk::ImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        if(addBlitFlags)
        {
            ci.AddUsageFlagsBits(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc);
        }
        if(addSampledFlag)
        {
            ci.AddUsageFlagsBits(vk::ImageUsageFlagBits::eSampled);
        }
        if(addStorageFlag)
        {
            ci.AddUsageFlagsBits(vk::ImageUsageFlagBits::eStorage);
        }
        return ci;
    }

    VkImageCreateInfo Image::CreateInfo::GetVulkanCreateInfo() const
    {
        return VkImageCreateInfo{
            .sType                 = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext                 = mPNext,
            .flags                 = mFlags,
            .imageType             = GetType(),
            .format                = mFormat,
            .extent                = mExtent,
            .mipLevels             = mMipLevelCount,
            .arrayLayers           = mArrayLayerCount,
            .samples               = mSampleCount,
            .tiling                = mTiling,
            .usage                 = mUsageFlags,
            .sharingMode           = mQueueFamilyIndices.empty() ? VkSharingMode::VK_SHARING_MODE_EXCLUSIVE : VkSharingMode::VK_SHARING_MODE_CONCURRENT,
            .queueFamilyIndexCount = (uint32_t)mQueueFamilyIndices.size(),
            .pQueueFamilyIndices   = mQueueFamilyIndices.data(),
            .initialLayout         = mInitialLayout,
        };
    }
}