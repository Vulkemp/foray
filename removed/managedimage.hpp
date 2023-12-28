#pragma once
#include "../basics.hpp"
#include "../mem.hpp"
#include "../vulkan.hpp"
#include "commandbuffer.hpp"
#include "context.hpp"
#include "managedresource.hpp"
#include <optional>

namespace foray::core {

    /// @brief Wraps allocation and lifetime functionality of vk::Image
    class ManagedImageOld : public VulkanResource<vk::ObjectType::VK_OBJECT_TYPE_IMAGE>
    {
      public:
        virtual ~ManagedImageOld();

        /// @brief Combines all structs used for initialization
        struct CreateInfo
        {
            /// @brief Vulkan Image CreateInfo
            VkImageCreateInfo ImageCI{};
            /// @brief If set to true, image view is created
            bool CreateImageView = true;
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
            CreateInfo(VkImageUsageFlags usage, vk::Format format, const VkExtent2D& extent, std::string_view name = "Unnamed Image");
        };

        /// @brief Allocates image, creates image, creates imageview
        /// @param context Requires Allocator, DispatchTable, Device
        ManagedImageOld(Context* context, const CreateInfo& createInfo);

        /// @brief Uses stored create info to recreate vulkan image with a new size.
        CreateInfo GetInfoForResize(const vk::Extent3D& newextent);
        /// @brief Uses stored create info to recreate vulkan image with a new size.
        CreateInfo GetInfoForResize(const VkExtent2D& newextent);

        /// @brief Shorthand using common values. See CreateInfo for information
        /// @param context Requires Allocator, DispatchTable, Device
        ManagedImageOld(Context* context, VkImageUsageFlags usage, vk::Format format, const VkExtent2D& extent, std::string_view name = "Unnamed Image");

        /// @brief Helper struct translated to a VkImageMemoryBarrier2 struct for one-time layout transitions
        struct QuickTransition
        {
            VkPipelineStageFlags SrcStageMask{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
            VkAccessFlags        SrcAccessMask{0};
            VkPipelineStageFlags DstStageMask{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
            VkAccessFlags        DstAccessMask{0};
            VkImageLayout        OldLayout{VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED};
            VkImageLayout        NewLayout{VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED};
            vk::ImageAspectFlags   AspectMask{VK_IMAGE_ASPECT_COLOR_BIT};
        };

        /// @brief Simple layout transition.
        /// @param commandBuffer If set, writes the transition to the commandbuffer. If not set, uses host synchronized temporary command buffer instead (slow)
        virtual void TransitionLayout(ManagedImageOld::QuickTransition& quickTransition, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

        /// @brief Creates a staging buffer, writes staging buffer, transitions image layout to transfer destination optimal,
        /// copies staging buffer to device local memory, transforms layout back to layoutAfterWrite parameter.
        /// @param data The data to write
        /// @param size Size of the image
        /// @param layoutAfterWrite The layout that the image is transitioned to after it has been written.
        /// @param imageCopy Specify how exactly the image is copied.
        void WriteDeviceLocalData(const void* data, size_t size, VkImageLayout layoutAfterWrite, VkBufferImageCopy& imageCopy);
        void WriteDeviceLocalData(HostSyncCommandBuffer& cmdBuffer, const void* data, size_t size, VkImageLayout layoutAfterWrite, VkBufferImageCopy& imageCopy);

        /// @brief See other overload for description. Omits image copy region and assumes a set of default values to write a simple
        /// image (no mimap, no layers) completely.
        void WriteDeviceLocalData(const void* data, size_t size, VkImageLayout layoutAfterWrite);
        void WriteDeviceLocalData(HostSyncCommandBuffer& cmdBuffer, const void* data, size_t size, VkImageLayout layoutAfterWrite);

        FORAY_GETTER_CR(CreateInfo)
        FORAY_GETTER_V(Context)
        FORAY_GETTER_V(Image)
        FORAY_GETTER_V(ImageView)
        FORAY_GETTER_V(Allocation)
        FORAY_GETTER_CR(AllocInfo)
        FORAY_GETTER_V(Format)
        FORAY_GETTER_CR(Extent3D)
        inline VkExtent2D GetExtent2D() const { return VkExtent2D{mExtent3D.width, mExtent3D.height}; }

        virtual void SetName(std::string_view name) override;

        vk::SampleCountFlagBits GetSampleCount() const { return mCreateInfo.ImageCI.samples; }

      protected:
        Context*          mContext{};
        CreateInfo        mCreateInfo;
        vk::Image           mImage{};
        vk::ImageView       mImageView{};
        vk::Format          mFormat{};
        VmaAllocation     mAllocation{};
        VmaAllocationInfo mAllocInfo{};
        VkDeviceSize      mSize{};
        vk::Extent3D        mExtent3D{};

        void CheckImageFormatSupport(const CreateInfo& createInfo);
        void UpdateDebugNames();
    };

    class Local_ManagedImageOld : public Local<ManagedImageOld>
    {
      public:
        void New(core::Context* context, const ManagedImageOld::CreateInfo& createInfo);
        void Resize(VkExtent2D extent);
        void Resize(vk::Extent3D extent);
    };
}  // namespace foray::core