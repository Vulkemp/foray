#pragma once
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include "foray_commandbuffer.hpp"
#include "foray_deviceresource.hpp"
#include "foray_context.hpp"
#include <optional>

namespace foray::core {
    class ManagedImage : public DeviceResourceBase
    {
      public:
        inline ManagedImage() { mName = "Unnamed Image"; }
        inline virtual ~ManagedImage() { Destroy(); }

        struct CreateInfo
        {
            VkImageCreateInfo       ImageCI{};
            VkImageViewCreateInfo   ImageViewCI{};
            VmaAllocationCreateInfo AllocCI{};
            std::string             Name{"UnnamedImage"};

            CreateInfo();
            CreateInfo(std::string name, VkImageUsageFlags usage, VkFormat format, const VkExtent3D& extent);
        };

        struct QuickTransition
        {
            VkPipelineStageFlags SrcStageMask{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
            VkAccessFlags        SrcAccessMask{0};
            VkPipelineStageFlags DstStageMask{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
            VkAccessFlags        DstAccessMask{0};
            VkImageLayout        OldLayout{VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED};
            VkImageLayout        NewLayout{VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED};
            VkImageAspectFlags   AspectMask{VK_IMAGE_ASPECT_COLOR_BIT};
        };

        virtual void Create(Context* context, CreateInfo createInfo);

        /// @brief Uses stored create info to recreate vulkan image.
        virtual void Resize(VkExtent3D newextent);

        virtual void Create(Context*         context,
                            VmaMemoryUsage           memoryUsage,
                            VmaAllocationCreateFlags flags,
                            VkExtent3D               extent,
                            VkImageUsageFlags        usage,
                            VkFormat                 format,
                            VkImageLayout            initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                            VkImageAspectFlags       aspectMask    = VK_IMAGE_ASPECT_COLOR_BIT,
                            std::string_view         name          = "UnnamedImage");

        /// @brief Simple layout transition.
        /// @param newLayout - The new layout for the image.
        virtual void TransitionLayout(ManagedImage::QuickTransition& quickTransition, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

        /// @brief Creates a staging buffer, writes staging buffer, transitions image layout to transfer destination optimal,
        /// copies staging buffer to device local memory, transforms layout back to layoutAfterWrite parameter.
        /// The functions assumes you are writing the complete image, with n
        /// @param data - The data to write,
        /// @param size - Size of the image
        /// @param layoutAfterWrite - The layout that the image is transitioned to after it has been written.
        /// @param imageCopy - Specify how exactly the image is copied.
        void WriteDeviceLocalData(const void* data, size_t size, VkImageLayout layoutAfterWrite, VkBufferImageCopy& imageCopy);
        void WriteDeviceLocalData(HostCommandBuffer& cmdBuffer, const void* data, size_t size, VkImageLayout layoutAfterWrite, VkBufferImageCopy& imageCopy);

        /// @brief See other overload for description. Omits image copy region and assumes a set of default values to write a simple
        /// image (no mimap, no layers) completely.
        void WriteDeviceLocalData(const void* data, size_t size, VkImageLayout layoutAfterWrite);
        void WriteDeviceLocalData(HostCommandBuffer& cmdBuffer, const void* data, size_t size, VkImageLayout layoutAfterWrite);

        virtual void Destroy() override;
        virtual bool Exists() const override { return mAllocation; }

        FORAY_PROPERTY_CGET(Image)
        FORAY_PROPERTY_CGET(ImageView)
        FORAY_PROPERTY_CGET(Allocation)
        FORAY_PROPERTY_CGET(AllocInfo)
        FORAY_PROPERTY_CGET(Format)
        FORAY_PROPERTY_CGET(Extent3D)
        FORAY_PROPERTY_GET(Name)
        FORAY_PROPERTY_CGET(Name)

        virtual ManagedImage& SetName(std::string_view name) override;

        VkSampleCountFlagBits GetSampleCount() { return mCreateInfo.ImageCI.samples; }

      protected:
        Context*  mContext{};
        CreateInfo        mCreateInfo;
        VkImage           mImage{};
        VkImageView       mImageView{};
        VkFormat          mFormat{};
        VmaAllocation     mAllocation{};
        VmaAllocationInfo mAllocInfo{};
        VkDeviceSize      mSize{};
        VkExtent3D        mExtent3D{};

        void CheckImageFormatSupport(CreateInfo& createInfo);
        void UpdateDebugNames();
    };
}  // namespace foray::core