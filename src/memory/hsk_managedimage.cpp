#include "hsk_managedimage.hpp"
#include "../hsk_vkHelpers.hpp"
#include "hsk_managedbuffer.hpp"
#include "hsk_singletimecommandbuffer.hpp"
#include "hsk_vmaHelpers.hpp"

namespace hsk {
    ManagedImage::CreateInfo::CreateInfo()
    {
        ImageCI.sType     = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ImageViewCI.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    }

    void ManagedImage::Create(const VkContext* context, CreateInfo createInfo)
    {
        if(mContext == nullptr)
        {
            // store create info during first creation
            mCreateInfo = createInfo;
        }
        mContext = context;

        // extract import image infos
        mName     = createInfo.Name;
        mFormat   = createInfo.ImageCI.format;
        mExtent3D = createInfo.ImageCI.extent;

        CheckImageFormatSupport(createInfo);

        // create image
        AssertVkResult(vmaCreateImage(mContext->Allocator, &createInfo.ImageCI, &createInfo.AllocCI, &mImage, &mAllocation, &mAllocInfo));

        // update image in image view create info
        createInfo.ImageViewCI.image = mImage;
        AssertVkResult(vkCreateImageView(mContext->Device, &createInfo.ImageViewCI, nullptr, &mImageView));

        // attach debug information to iamge
        SetupDebugInfo(createInfo);
    }

    void ManagedImage::Recreate()
    {
        Assert(mContext != nullptr, "Attempted to recreate image before initial creation!");
        Create(mContext, mCreateInfo);
    }

    void ManagedImage::Create(const VkContext*         context,
                              VmaMemoryUsage           memoryUsage,
                              VmaAllocationCreateFlags flags,
                              VkExtent3D               extent,
                              VkImageUsageFlags        usage,
                              VkFormat                 format,
                              VkImageLayout            initialLayout,
                              VkImageAspectFlags       aspectMask,
                              std::string_view         name)
    {
        CreateInfo createInfo;
        createInfo.AllocCI.flags = flags;
        createInfo.AllocCI.usage = memoryUsage;

        createInfo.ImageCI.imageType     = VK_IMAGE_TYPE_2D;
        createInfo.ImageCI.format        = format;
        createInfo.ImageCI.extent        = extent;
        createInfo.ImageCI.mipLevels     = 1;
        createInfo.ImageCI.arrayLayers   = 1;
        createInfo.ImageCI.samples       = VK_SAMPLE_COUNT_1_BIT;
        createInfo.ImageCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
        createInfo.ImageCI.usage         = usage;
        createInfo.ImageCI.initialLayout = initialLayout;

        createInfo.ImageViewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.ImageViewCI.format                          = format;
        createInfo.ImageViewCI.subresourceRange                = {};
        createInfo.ImageViewCI.subresourceRange.aspectMask     = aspectMask;
        createInfo.ImageViewCI.subresourceRange.baseMipLevel   = 0;
        createInfo.ImageViewCI.subresourceRange.levelCount     = 1;
        createInfo.ImageViewCI.subresourceRange.baseArrayLayer = 0;
        createInfo.ImageViewCI.subresourceRange.layerCount     = 1;

        createInfo.Name = name;

        Create(context, createInfo);
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
        barrier.oldLayout           = transitionInfo.OldImageLayout != VK_IMAGE_LAYOUT_UNDEFINED ? transitionInfo.OldImageLayout : mImageLayout;
        barrier.newLayout           = transitionInfo.NewImageLayout;
        barrier.srcQueueFamilyIndex = transitionInfo.SrcQueueFamilyIndex;
        barrier.dstQueueFamilyIndex = transitionInfo.DstQueueFamilyIndex;
        barrier.image               = mImage;
        barrier.subresourceRange    = transitionInfo.SubresourceRange;
        barrier.srcAccessMask       = transitionInfo.BarrierSrcAccessMask;
        barrier.dstAccessMask       = transitionInfo.BarrierDstAccessMask;

        vkCmdPipelineBarrier(commandBuffer, transitionInfo.SrcStage, transitionInfo.DstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        // update image layout
        mImageLayout = barrier.newLayout;
    }

    void ManagedImage::WriteDeviceLocalData(void* data, size_t size, VkImageLayout layoutAfterWrite, VkBufferImageCopy& imageCopy)
    {
        // create staging buffer
        ManagedBuffer stagingBuffer;
        stagingBuffer.CreateForStaging(mContext, size, data);

        // transform image layout to write dst
        TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // copy staging buffer data into device local memory
        SingleTimeCommandBuffer singleTimeCmdBuf;
        singleTimeCmdBuf.Create(mContext);
        vkCmdCopyBufferToImage(singleTimeCmdBuf.GetCommandBuffer(), stagingBuffer.GetBuffer(), mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
        singleTimeCmdBuf.Flush(true);

        // reset image layout
        TransitionLayout(layoutAfterWrite);
    }

    void ManagedImage::WriteDeviceLocalData(void* data, size_t size, VkImageLayout layoutAfterWrite)
    {
        // specify default copy region
        VkBufferImageCopy region{};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = {0, 0, 0};
        region.imageExtent                     = mExtent3D;
        WriteDeviceLocalData(data, size, layoutAfterWrite, region);
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

    void ManagedImage::SetupDebugInfo(CreateInfo& createInfo)
    {
        if(mContext->DebugEnabled)
        {
            // Set a name on the image
            const VkDebugUtilsObjectNameInfoEXT imageNameInfo = {
                VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,  // sType
                NULL,                                                // pNext
                VK_OBJECT_TYPE_IMAGE,                                // objectType
                (uint64_t)mImage,                                    // objectHandle
            };

            mContext->DispatchTable.setDebugUtilsObjectNameEXT(&imageNameInfo);
        }
    }

    void ManagedImage::CheckImageFormatSupport(CreateInfo& createInfo)
    {
        VkImageFormatProperties props{};
        // check if image format together with required flags and usage is supported.
        AssertVkResult(vkGetPhysicalDeviceImageFormatProperties(mContext->PhysicalDevice, createInfo.ImageCI.format, createInfo.ImageCI.imageType, createInfo.ImageCI.tiling,
                                                                createInfo.ImageCI.usage, createInfo.ImageCI.flags, &props));
    }
}  // namespace hsk