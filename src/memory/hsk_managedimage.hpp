#pragma once
#include "../hsk_basics.hpp"
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include "../base/hsk_vkcontext.hpp"

namespace hsk {
    class ManagedImage : public NoMoveDefaults
    {
      public:
        inline ManagedImage() {}
        inline ~ManagedImage() { Destroy(); }

        struct CreateInfo
        {
            VkImageCreateInfo       ImageCI{};
            VkImageViewCreateInfo   ImageViewCI{};
            VmaAllocationCreateInfo AllocCI{};

            CreateInfo();
        };

        virtual void Create(const VkContext* context, CreateInfo& createInfo);

        virtual void Create(const VkContext*         context,
                              VmaMemoryUsage           memoryUsage,
                              VmaAllocationCreateFlags flags,
                              VkExtent3D               extent,
                              VkImageUsageFlags        usage,
                              VkFormat                 format,
                              VkImageLayout            initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                              VkImageAspectFlags       aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

        /// @brief When doing a layout transition, specify the transition parameters.
        struct LayoutTransitionInfo
        {
            /// @brief New image layout is mandatory!
            VkImageLayout        NewImageLayout{VK_IMAGE_LAYOUT_UNDEFINED};
            VkAccessFlags        BarrierSrcAccessMask{0};
            VkAccessFlags        BarrierDstAccessMask{0};
            VkPipelineStageFlags SrcStage{};
            VkPipelineStageFlags DstStage{};
            /// @brief If no command buffer is passed, a single time command buffer will be created to transfer the layout.
            VkCommandBuffer         CommandBuffer{nullptr};
            uint32_t                SrcQueueFamilyIndex{VK_QUEUE_FAMILY_IGNORED};
            uint32_t                DstQueueFamilyIndex{VK_QUEUE_FAMILY_IGNORED};
            VkImageSubresourceRange SubresourceRange{
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            };
            /// @brief If the CommandBuffer member is left to nullptr, a single time command buffer is generated with this level.
            VkCommandBufferLevel CommandBufferLevel{VK_COMMAND_BUFFER_LEVEL_PRIMARY};
        };

        /// @brief Simple layout transition.
        /// @param newLayout - The new layout for the image.
        virtual void TransitionLayout(VkImageLayout newLayout);
        /// @brief Detailed layout transition.
        virtual void TransitionLayout(LayoutTransitionInfo& transitionInfo);

        /// @brief Creates a staging buffer, writes staging buffer, transitions image layout to transfer destination optimal,
        /// copies staging buffer to device local memory, transforms layout back to layoutAfterWrite parameter.
        /// The functions assumes you are writing the complete image, with n
        /// @param data - The data to write,
        /// @param size - Size of the image
        /// @param layoutAfterWrite - The layout that the image is transitioned to after it has been written.
        /// @param imageCopy - Specify how exactly the image is copied.
        void WriteDeviceLocalData(void* data, size_t size, VkImageLayout layoutAfterWrite, VkBufferImageCopy& imageCopy);

        /// @brief See other overload for description. Omits image copy region and assumes a set of default values to write a simple
        /// image (no mimap, no layers) completely.
        void WriteDeviceLocalData(void* data, size_t size, VkImageLayout layoutAfterWrite);

        virtual void Destroy();

        HSK_PROPERTY_CGET(Image)
        HSK_PROPERTY_CGET(ImageView)
        HSK_PROPERTY_CGET(ImageLayout)
        HSK_PROPERTY_CGET(Allocation)
        HSK_PROPERTY_CGET(AllocInfo)
        HSK_PROPERTY_CGET(Format)
        HSK_PROPERTY_CGET(Extent3D)

      protected:
        const VkContext*  mContext{};
        VkImage           mImage{};
        VkImageView       mImageView{};
        VkImageLayout     mImageLayout{};
        VkFormat          mFormat{};
        VmaAllocation     mAllocation{};
        VmaAllocationInfo mAllocInfo{};
        VkExtent3D        mExtent3D{};
    };
}  // namespace hsk