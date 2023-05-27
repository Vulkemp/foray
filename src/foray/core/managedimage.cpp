#include "managedimage.hpp"
#include "../mem.hpp"
#include "../util/fmtutilities.hpp"
#include "commandbuffer.hpp"
#include "managedbuffer.hpp"

namespace foray::core {
    ManagedImage::CreateInfo::CreateInfo()
        : ImageCI{.sType       = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                  .imageType   = VkImageType::VK_IMAGE_TYPE_2D,
                  .mipLevels   = 1U,
                  .arrayLayers = 1U,
                  .samples     = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
                  .tiling      = VkImageTiling::VK_IMAGE_TILING_OPTIMAL}
        , ImageViewCI{.sType            = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                      .viewType         = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D,
                      .subresourceRange = VkImageSubresourceRange{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1U, .layerCount = 1U}}
        , AllocCI{.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE}
    {
    }

    ManagedImage::CreateInfo::CreateInfo(VkImageUsageFlags usage, VkFormat format, const VkExtent2D& extent, std::string_view name) : CreateInfo()
    {
        ImageCI.usage      = usage;
        ImageCI.format     = format;
        ImageViewCI.format = format;
        ImageCI.extent     = VkExtent3D{.width = extent.width, .height = extent.height, .depth = 1};
        Name               = name;
    }

    ManagedImage::ManagedImage(Context* context, const CreateInfo& createInfo) : VulkanResource<VK_OBJECT_TYPE_IMAGE>("Unnamed Buffer")
    {
        mContext    = context;
        mCreateInfo = createInfo;

        Assert(createInfo.ImageCI.extent.width > 0 && createInfo.ImageCI.extent.height > 0 && createInfo.ImageCI.extent.depth > 0, "Extent must be > 0");

        // extract import image infos
        mName     = mCreateInfo.Name;
        mFormat   = mCreateInfo.ImageCI.format;
        mExtent3D = mCreateInfo.ImageCI.extent;

        CheckImageFormatSupport(mCreateInfo);

        // create image
        AssertVkResult(vmaCreateImage(mContext->Allocator, &mCreateInfo.ImageCI, &mCreateInfo.AllocCI, &mImage, &mAllocation, &mAllocInfo));
        mSize = mAllocInfo.size;

        if(mCreateInfo.CreateImageView)
        {
            // update image in image view create info
            mCreateInfo.ImageViewCI.image = mImage;
            AssertVkResult(mContext->DispatchTable().createImageView(&mCreateInfo.ImageViewCI, nullptr, &mImageView));
        }

#if FORAY_DEBUG
        // attach debug information to iamge
        if(mName.size())
        {
            UpdateDebugNames();
        }
#endif
    }

    ManagedImage::CreateInfo ManagedImage::GetInfoForResize(const VkExtent2D& newExtent)
    {
        Assert(Exists(), "Attempted to resize image before initial creation!");
        mCreateInfo.ImageCI.extent = VkExtent3D{.width = newExtent.width, .height = newExtent.height, .depth = 1};
        return mCreateInfo;
    }
    ManagedImage::CreateInfo ManagedImage::GetInfoForResize(const VkExtent3D& newExtent)
    {
        Assert(Exists(), "Attempted to resize image before initial creation!");
        mCreateInfo.ImageCI.extent = newExtent;
        return mCreateInfo;
    }

    ManagedImage::ManagedImage(Context* context, VkImageUsageFlags usage, VkFormat format, const VkExtent2D& extent, std::string_view name)
        : ManagedImage(context, CreateInfo(usage, format, extent, name))
    {
    }

    void ManagedImage::TransitionLayout(ManagedImage::QuickTransition& quickTransition, VkCommandBuffer commandBuffer)
    {
        VkImageMemoryBarrier barrier{
            .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = quickTransition.SrcAccessMask,
            .dstAccessMask       = quickTransition.DstAccessMask,
            .oldLayout           = quickTransition.OldLayout,
            .newLayout           = quickTransition.NewLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = mImage,
            .subresourceRange = VkImageSubresourceRange{.aspectMask = quickTransition.AspectMask, .levelCount = VK_REMAINING_MIP_LEVELS, .layerCount = VK_REMAINING_ARRAY_LAYERS}};

        Local<core::HostSyncCommandBuffer> hostCmdBuffer;

        if(!commandBuffer)
        {
            hostCmdBuffer.New(mContext);
            hostCmdBuffer->Begin();
            commandBuffer = hostCmdBuffer.GetRef();
        }

        vkCmdPipelineBarrier(commandBuffer, quickTransition.SrcStageMask, quickTransition.DstStageMask, 0, 0, nullptr, 0, nullptr, 1U, &barrier);

        if(hostCmdBuffer)
        {
            hostCmdBuffer->SubmitAndWait();
        }
    }

    void ManagedImage::WriteDeviceLocalData(const void* data, size_t size, VkImageLayout layoutAfterWrite, VkBufferImageCopy& imageCopy)
    {
        HostSyncCommandBuffer cmdBuffer(mContext);
        WriteDeviceLocalData(cmdBuffer, data, size, layoutAfterWrite, imageCopy);
    }
    void ManagedImage::WriteDeviceLocalData(HostSyncCommandBuffer& cmdBuffer, const void* data, size_t size, VkImageLayout layoutAfterWrite, VkBufferImageCopy& imageCopy)
    {
        // create staging buffer
        ManagedBuffer stagingBuffer(mContext, ManagedBuffer::CreateForStaging(size, fmt::format("Staging for {}", GetName())));
        stagingBuffer.MapAndWrite(data, size);

        cmdBuffer.Begin();

        // transform image layout to write dst

        QuickTransition transition{.SrcStageMask  = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   .DstStageMask  = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   .DstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT,
                                   .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};
        TransitionLayout(transition, cmdBuffer);

        // copy staging buffer data into device local memory
        vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.GetBuffer(), mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

        if(layoutAfterWrite)
        {
            // reset image layout   // reset image layout
            QuickTransition transition{.SrcStageMask  = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
                                       .SrcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT,
                                       .DstStageMask  = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                       .OldLayout     = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       .NewLayout     = layoutAfterWrite};
            TransitionLayout(transition, cmdBuffer);
        }

        cmdBuffer.SubmitAndWait();
    }

    void ManagedImage::WriteDeviceLocalData(const void* data, size_t size, VkImageLayout layoutAfterWrite)
    {
        HostSyncCommandBuffer cmdBuffer(mContext);
        WriteDeviceLocalData(cmdBuffer, data, size, layoutAfterWrite);
    }
    void ManagedImage::WriteDeviceLocalData(HostSyncCommandBuffer& cmdBuffer, const void* data, size_t size, VkImageLayout layoutAfterWrite)
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

    ManagedImage::~ManagedImage()
    {
        if(mAllocation)
        {
            if(!!mImageView)
            {
                mContext->DispatchTable().destroyImageView(mImageView, nullptr);
                mImageView = nullptr;
            }
            vmaDestroyImage(mContext->Allocator, mImage, mAllocation);
            mImage      = nullptr;
            mAllocation = nullptr;
            mAllocInfo  = VmaAllocationInfo{};
        }
    }

    void ManagedImage::CheckImageFormatSupport(const CreateInfo& createInfo)
    {
        VkImageFormatProperties props{};
        // check if image format together with required flags and usage is supported.
        AssertVkResult(vkGetPhysicalDeviceImageFormatProperties(mContext->VkPhysicalDevice(), createInfo.ImageCI.format, createInfo.ImageCI.imageType, createInfo.ImageCI.tiling,
                                                                createInfo.ImageCI.usage, createInfo.ImageCI.flags, &props));
    }

    void ManagedImage::SetName(std::string_view name)
    {
        mName = name;
#if FORAY_DEBUG
        if(mAllocation)
        {
            UpdateDebugNames();
        }
#endif
    }

    void ManagedImage::UpdateDebugNames()
    {
        {  // Image
            std::string debugName = fmt::format("ManImg \"{}\" ({})", mName, util::PrintSize(mSize));
            SetObjectName(mContext, mImage, debugName, false);
            vmaSetAllocationName(mContext->Allocator, mAllocation, debugName.c_str());
        }
        if(!!mImageView)
        {  // Image View
            std::string debugName = fmt::format("ManImgView \"{}\"", mName);
            SetVulkanObjectName(mContext, VkObjectType::VK_OBJECT_TYPE_IMAGE_VIEW, mImageView, debugName);
        }
    }


    void Local_ManagedImage::New(core::Context* context, const ManagedImage::CreateInfo& createInfo)
    {
        Local<ManagedImage>::New(context, createInfo);
    }

    void Local_ManagedImage::Resize(VkExtent2D extent)
    {
        Assert(Exists());
        core::Context*           context = GetRef().GetContext();
        ManagedImage::CreateInfo ci      = GetRef().GetInfoForResize(extent);
        New(context, ci);
    }

    void Local_ManagedImage::Resize(VkExtent3D extent)
    {
        Assert(Exists());
        core::Context*           context = GetRef().GetContext();
        ManagedImage::CreateInfo ci      = GetRef().GetInfoForResize(extent);
        New(context, ci);
    }

}  // namespace foray::core