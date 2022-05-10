#include "hsk_intermediateBuffer.hpp"

namespace hsk {
    IntermediateBuffer::CreateInfo::CreateInfo() { ImageCI.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO; }

    void IntermediateBuffer::Init(const CreateInfo& createInfo)
    {
        mFormat = createInfo.ImageCI.format;
        vmaCreateImage(mAllocator, &createInfo.ImageCI, &createInfo.AllocCI, &mImage, &mAllocation, &mAllocInfo);

        VkImageViewCreateInfo imageViewCI{};
        imageViewCI.sType                           = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCI.format                          = mFormat;
        imageViewCI.components                      = {};
        imageViewCI.image                           = mImage;
        imageViewCI.subresourceRange.aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCI.subresourceRange.baseMipLevel   = 0;
        imageViewCI.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
        imageViewCI.subresourceRange.baseArrayLayer = 0;
        imageViewCI.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        imageViewCI.viewType                        = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

        vkCreateImageView(mDevice, &imageViewCI, nullptr, &mImageView);
    }

    void IntermediateBuffer::Destroy()
    {
        if(mAllocation)
        {
            vkDestroyImageView(mDevice, mImageView, nullptr);
            vmaDestroyImage(mAllocator, mImage, mAllocation);
            mImage      = nullptr;
            mAllocation = nullptr;
            mAllocInfo  = VmaAllocationInfo{};
        }
    }
}  // namespace hsk