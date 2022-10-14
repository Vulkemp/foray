#include "foray_managedimage.hpp"
#include "../util/foray_fmtutilities.hpp"
#include "foray_commandbuffer.hpp"
#include "foray_managedbuffer.hpp"

namespace foray::core {
    ManagedImage::CreateInfo::CreateInfo()
    {
        // required: initial layout, usage flags, the format (also for the view) and the
        // extent
        // others as needed.

        ImageCI.sType     = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ImageViewCI.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        AllocCI.flags     = 0;
        AllocCI.usage     = VMA_MEMORY_USAGE_AUTO;

        ImageCI.imageType     = VK_IMAGE_TYPE_2D;
        ImageCI.format        = VK_FORMAT_UNDEFINED;
        ImageCI.mipLevels     = 1;
        ImageCI.arrayLayers   = 1;
        ImageCI.samples       = VK_SAMPLE_COUNT_1_BIT;
        ImageCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
        ImageCI.usage         = 0;
        ImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        ImageViewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        ImageViewCI.format                          = VK_FORMAT_UNDEFINED;
        ImageViewCI.subresourceRange                = {};
        ImageViewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageViewCI.subresourceRange.baseMipLevel   = 0;
        ImageViewCI.subresourceRange.levelCount     = 1;
        ImageViewCI.subresourceRange.baseArrayLayer = 0;
        ImageViewCI.subresourceRange.layerCount     = 1;
    }

    ManagedImage::CreateInfo::CreateInfo(std::string name, VkImageLayout initialLayout, VkImageUsageFlags usage, VkFormat format, const VkExtent3D& extent) : CreateInfo()
    {
        ImageCI.initialLayout = initialLayout;
        ImageCI.usage         = usage;
        ImageCI.format        = format;
        ImageViewCI.format    = format;
        ImageCI.extent        = extent;
        Name                  = name;
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
        mImageLayout = createInfo.ImageCI.initialLayout;
        mSize        = mAllocInfo.size;

        // update image in image view create info
        createInfo.ImageViewCI.image = mImage;
        AssertVkResult(vkCreateImageView(mContext->Device, &createInfo.ImageViewCI, nullptr, &mImageView));

        // attach debug information to iamge
        if(mName.size() && mContext->DebugEnabled)
        {
            UpdateDebugNames();
        }
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

        createInfo.Name = name == "UnnamedImage" ? GetName() : name;

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
        else if(mImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
        {
            transitionInfo.BarrierSrcAccessMask = 0;
            transitionInfo.BarrierDstAccessMask = 0;

            transitionInfo.SrcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            transitionInfo.DstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
        else
        {
            Exception::Throw("No simple translation for this layout available!");
        }
        transitionInfo.NewImageLayout = newLayout;
        TransitionLayout(transitionInfo);
    }


    void ManagedImage::TransitionLayout(LayoutTransitionInfo& transitionInfo)
    {
        bool            createTemporaryCommandBuffer = false;
        VkCommandBuffer commandBuffer;
        HostCommandBuffer   tempCmdBuffer;
        if(transitionInfo.CommandBuffer == nullptr)
        {
            createTemporaryCommandBuffer = true;
            tempCmdBuffer.Create(mContext);
            tempCmdBuffer.Begin();
            commandBuffer = tempCmdBuffer;
        }
        else
        {
            commandBuffer = transitionInfo.CommandBuffer;
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout           = transitionInfo.OldImageLayout.has_value() ? transitionInfo.OldImageLayout.value() : mImageLayout;
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
        if(tempCmdBuffer.Exists())
        {
            tempCmdBuffer.SubmitAndWait();
            tempCmdBuffer.Destroy();
        }
    }

    void ManagedImage::WriteDeviceLocalData(const void* data, size_t size, VkImageLayout layoutAfterWrite, VkBufferImageCopy& imageCopy)
    {
        HostCommandBuffer cmdBuffer;
        cmdBuffer.Create(mContext);
        WriteDeviceLocalData(cmdBuffer, data, size, layoutAfterWrite, imageCopy);
    }
    void ManagedImage::WriteDeviceLocalData(HostCommandBuffer& cmdBuffer, const void* data, size_t size, VkImageLayout layoutAfterWrite, VkBufferImageCopy& imageCopy)
    {
        // create staging buffer
        ManagedBuffer stagingBuffer;
        stagingBuffer.CreateForStaging(mContext, size, data, fmt::format("Staging for {}", GetName()));

        cmdBuffer.Begin();

        // transform image layout to write dst
        LayoutTransitionInfo transitionInfo;
        transitionInfo.CommandBuffer        = cmdBuffer.GetCommandBuffer();
        transitionInfo.NewImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        transitionInfo.BarrierSrcAccessMask = 0;
        transitionInfo.BarrierDstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        transitionInfo.SrcStage             = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        transitionInfo.DstStage             = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;

        TransitionLayout(transitionInfo);

        // copy staging buffer data into device local memory
        vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.GetBuffer(), mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

        if(layoutAfterWrite)
        {
            // reset image layout
            transitionInfo.OldImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            transitionInfo.NewImageLayout       = layoutAfterWrite;
            transitionInfo.BarrierSrcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
            transitionInfo.BarrierDstAccessMask = 0;
            transitionInfo.SrcStage             = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;
            transitionInfo.DstStage             = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

            TransitionLayout(transitionInfo);
        }

        cmdBuffer.Submit();
    }

    void ManagedImage::WriteDeviceLocalData(const void* data, size_t size, VkImageLayout layoutAfterWrite)
    {
        HostCommandBuffer cmdBuffer;
        cmdBuffer.Create(mContext);
        WriteDeviceLocalData(cmdBuffer, data, size, layoutAfterWrite);
    }
    void ManagedImage::WriteDeviceLocalData(HostCommandBuffer& cmdBuffer, const void* data, size_t size, VkImageLayout layoutAfterWrite)
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
        WriteDeviceLocalData(cmdBuffer, data, size, layoutAfterWrite, region);
    }

    void ManagedImage::Destroy()
    {
        if(mAllocation)
        {
            vkDestroyImageView(mContext->Device, mImageView, nullptr);
            vmaDestroyImage(mContext->Allocator, mImage, mAllocation);
            mImage       = nullptr;
            mAllocation  = nullptr;
            mAllocInfo   = VmaAllocationInfo{};
            mImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }

    void ManagedImage::CheckImageFormatSupport(CreateInfo& createInfo)
    {
        VkImageFormatProperties props{};
        // check if image format together with required flags and usage is supported.
        AssertVkResult(vkGetPhysicalDeviceImageFormatProperties(mContext->PhysicalDevice, createInfo.ImageCI.format, createInfo.ImageCI.imageType, createInfo.ImageCI.tiling,
                                                                createInfo.ImageCI.usage, createInfo.ImageCI.flags, &props));
    }

    ManagedImage& ManagedImage::SetName(std::string_view name)
    {
        mName = name;
        if(mAllocation && mContext->DebugEnabled)
        {
            UpdateDebugNames();
        }
        return *this;
    }

    void ManagedImage::UpdateDebugNames()
    {
        {  // Image
            std::string                   debugName = fmt::format("ManImg \"{}\" ({})", mName, util::PrintSize(mSize));
            VkDebugUtilsObjectNameInfoEXT nameInfo{.sType        = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                                                   .pNext        = nullptr,
                                                   .objectType   = VkObjectType::VK_OBJECT_TYPE_IMAGE,
                                                   .objectHandle = reinterpret_cast<uint64_t>(mImage),
                                                   .pObjectName  = debugName.c_str()};
            mContext->DispatchTable.setDebugUtilsObjectNameEXT(&nameInfo);
            vmaSetAllocationName(mContext->Allocator, mAllocation, debugName.c_str());
        }
        {  // Image View
            std::string                   debugName = fmt::format("ManImgView \"{}\"", mName);
            VkDebugUtilsObjectNameInfoEXT nameInfo{.sType        = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                                                   .pNext        = nullptr,
                                                   .objectType   = VkObjectType::VK_OBJECT_TYPE_IMAGE_VIEW,
                                                   .objectHandle = reinterpret_cast<uint64_t>(mImageView),
                                                   .pObjectName  = debugName.c_str()};
            mContext->DispatchTable.setDebugUtilsObjectNameEXT(&nameInfo);
        }
    }


}  // namespace foray::core