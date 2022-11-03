#pragma once
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include "foray_commandbuffer.hpp"
#include "foray_context.hpp"
#include "foray_managedresource.hpp"
#include <optional>

namespace foray::core {

    /// @brief Wraps allocation and lifetime functionality of VkImage
    class ManagedImage : public VulkanResource<VkObjectType::VK_OBJECT_TYPE_IMAGE>
    {
      public:
        inline ManagedImage() : VulkanResource("Unnamed Image") {}
        inline virtual ~ManagedImage() { Destroy(); }

        /// @brief Combines all structs used for initialization
        struct CreateInfo
        {
            /// @brief Vulkan Image CreateInfo
            VkImageCreateInfo ImageCI{};
            /// @brief Vulkan ImageView CreateInfo
            VkImageViewCreateInfo ImageViewCI{};
            /// @brief Vma Allocation CreateInfo
            VmaAllocationCreateInfo AllocCI{};
            /// @brief Debug object name
            std::string Name{"Unnamed Image"};

            /// @brief Initiliazes .sType fields, chooses common defaults for everything else
            CreateInfo();
            /// @brief Shorthand for initializing commonly set values
            /// @param usage Image usage determines how vulkan can utilize/access the image
            /// @param format Pixel format
            /// @param extent 2D Size (depth = 1)
            /// @param name Debug object name
            CreateInfo(VkImageUsageFlags usage, VkFormat format, const VkExtent2D& extent, std::string_view name = "Unnamed Image");
        };

        /// @brief Allocates image, creates image, creates imageview
        /// @param context Requires Allocator, DispatchTable, Device
        virtual void Create(Context* context, CreateInfo createInfo);

        /// @brief Uses stored create info to recreate vulkan image with a new size.
        virtual void Resize(const VkExtent3D& newextent);
        /// @brief Uses stored create info to recreate vulkan image with a new size.
        virtual void Resize(const VkExtent2D& newextent);

        /// @brief Shorthand using common values. See CreateInfo for information
        /// @param context Requires Allocator, DispatchTable, Device
        virtual void Create(Context*           context,
                            VkImageUsageFlags  usage,
                            VkFormat           format,
                            const VkExtent2D&  extent,
                            std::string_view   name       = "Unnamed Image");

        /// @brief Helper struct translated to a VkImageMemoryBarrier2 struct for one-time layout transitions
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

        /// @brief Simple layout transition.
        /// @param commandBuffer If set, writes the transition to the commandbuffer. If not set, uses host synchronized temporary command buffer instead (slow)
        virtual void TransitionLayout(ManagedImage::QuickTransition& quickTransition, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

        /// @brief Creates a staging buffer, writes staging buffer, transitions image layout to transfer destination optimal,
        /// copies staging buffer to device local memory, transforms layout back to layoutAfterWrite parameter.
        /// @param data The data to write
        /// @param size Size of the image
        /// @param layoutAfterWrite The layout that the image is transitioned to after it has been written.
        /// @param imageCopy Specify how exactly the image is copied.
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
        FORAY_PROPERTY_CGET(Name)

        virtual void SetName(std::string_view name) override;

        VkSampleCountFlagBits GetSampleCount() { return mCreateInfo.ImageCI.samples; }

      protected:
        Context*          mContext{};
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