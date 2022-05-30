#include "hsk_managedimage.hpp"
#include "../hsk_vkHelpers.hpp"
#include "hsk_managedbuffer.hpp"
#include "hsk_vmaHelpers.hpp"
#include "hsk_singletimecommandbuffer.hpp"

namespace hsk {
    ManagedImage::CreateInfo::CreateInfo()
    {
        ImageCI.sType     = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ImageViewCI.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    }

    void ManagedImage::Create(const VkContext* context, const CreateInfo& createInfo)
    {
        mContext = context;

        // extract import image infos
        mFormat = createInfo.ImageCI.format;
        mExtent3D = createInfo.ImageCI.extent;

        // create image and view
        vmaCreateImage(mContext->Allocator, &createInfo.ImageCI, &createInfo.AllocCI, &mImage, &mAllocation, &mAllocInfo);
        vkCreateImageView(mContext->Device, &createInfo.ImageViewCI, nullptr, &mImageView);
    }


    void ManagedImage::TransitionLayout(VkImageLayout newLayout)
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


    void ManagedImage::TransitionLayout(LayoutTransitionInfo& transitionInfo)
    {
        bool            createTemporaryCommandBuffer = false;
        VkCommandBuffer commandBuffer;
        if(transitionInfo.CommandBuffer == nullptr)
        {
            createTemporaryCommandBuffer = true;
            commandBuffer                = CreateCommandBuffer(mContext->Device, mContext->CommandPool, transitionInfo.CommandBufferLevel, true);
        }
        else
        {
            commandBuffer = transitionInfo.CommandBuffer;
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout           = mImageLayout;
        barrier.newLayout           = transitionInfo.NewImageLayout;
        barrier.srcQueueFamilyIndex = transitionInfo.SrcQueueFamilyIndex;
        barrier.dstQueueFamilyIndex = transitionInfo.DstQueueFamilyIndex;
        barrier.image               = mImage;
        barrier.subresourceRange    = transitionInfo.SubresourceRange;

        vkCmdPipelineBarrier(commandBuffer, transitionInfo.SrcStage, transitionInfo.DstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void ManagedImage::WriteDeviceLocalData(void* data, size_t size, VkImageLayout layoutAfterWrite)
    {
        // create staging buffer
        ManagedBuffer stagingBuffer;
        VmaHelpers::CreateStagingBuffer(&stagingBuffer, mContext, data, size);

        // transform image layout to write dst
        TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // specify copy region
        VkBufferImageCopy region{};
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = mExtent3D;

        // copy staging buffer data into device local memory
        SingleTimeCommandBuffer singleTimeCmdBuf;
        singleTimeCmdBuf.Create(mContext);
        vkCmdCopyBufferToImage(singleTimeCmdBuf.GetCommandBuffer(), stagingBuffer.GetBuffer(), mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        singleTimeCmdBuf.Flush(true);

        // reset image layout
        TransitionLayout(layoutAfterWrite);

    }

    void ManagedImage::Destroy()
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