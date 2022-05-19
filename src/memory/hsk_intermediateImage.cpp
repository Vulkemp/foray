#include "hsk_intermediateImage.hpp"
#include "hsk_vmaHelpers.hpp"

namespace hsk {
    IntermediateImage::CreateInfo::CreateInfo() { ImageCI.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO; }

    void IntermediateImage::Init(const CreateInfo& createInfo)
    {
        mFormat = createInfo.ImageCI.format;
        vmaCreateImage(mContext->Allocator, &createInfo.ImageCI, &createInfo.AllocCI, &mImage, &mAllocation, &mAllocInfo);

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

        vkCreateImageView(mContext->Device, &imageViewCI, nullptr, &mImageView);
    }


    void IntermediateImage::TransitionLayout(VkImageLayout newLayout)
    {
        LayoutTransitionInfo transitionInfo;
        if(mImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            transitionInfo.BarrierSrcAccessMask = 0;
            transitionInfo.BarrierDstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            transitionInfo.SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            transitionInfo.DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if(mImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            transitionInfo.BarrierSrcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            transitionInfo.BarrierDstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            transitionInfo.SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            transitionInfo.DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            Exception::Throw("No simple translation for this layout available!");
        }
        TransitionLayout(transitionInfo);
    }


    void IntermediateImage::TransitionLayout(LayoutTransitionInfo& transitionInfo)
    {
        bool createTemporaryCommandBuffer = false;
        VkCommandBuffer commandBuffer;
        if(transitionInfo.CommandBuffer == nullptr)
        {
            createTemporaryCommandBuffer = true;
            commandBuffer = createCommandBuffer(mContext->Device, mContext->CommandPool, transitionInfo.CommandBufferLevel, true);
        }
        else
        {
            commandBuffer = transitionInfo.CommandBuffer;     
        }
        
        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = mImageLayout;
        barrier.newLayout                       = transitionInfo.NewImageLayout;
        barrier.srcQueueFamilyIndex             = transitionInfo.SrcQueueFamilyIndex;
        barrier.dstQueueFamilyIndex             = transitionInfo.DstQueueFamilyIndex;
        barrier.image                           = mImage;
        barrier.subresourceRange                = transitionInfo.SubresourceRange;

        vkCmdPipelineBarrier(commandBuffer, transitionInfo.SrcStage, transitionInfo.DstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        flushCommandBuffer(mContext->Device, mContext->CommandPool, commandBuffer, mContext->QueueGraphics, true);
    }

    void IntermediateImage::Destroy()
    {
        if(mAllocation)
        {
            vkDestroyImageView(mContext->Device, mImageView, nullptr);
            vmaDestroyImage(mContext->Allocator, mImage, mAllocation);
            mImage      = nullptr;
            mAllocation = nullptr;
            mAllocInfo  = VmaAllocationInfo{};
        }
    }
}  // namespace hsk