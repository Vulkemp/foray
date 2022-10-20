#pragma once
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include "foray_commandbuffer.hpp"
#include "foray_deviceresource.hpp"
#include "foray_vkcontext.hpp"
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
            CreateInfo(std::string name, VkImageLayout initialLayout, VkImageUsageFlags usage, VkFormat format, const VkExtent3D& extent);
        };

        struct QuickTransition
        {
            VkImageLayout        NewImageLayout{};
            VkAccessFlags        SrcMask{0};
            VkAccessFlags        DstMask{0};
            VkPipelineStageFlags SrcStage{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
            VkPipelineStageFlags DstStage{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
            VkImageAspectFlags   AspectMask{VK_IMAGE_ASPECT_COLOR_BIT};
        };

        virtual void Create(const VkContext* context, CreateInfo createInfo);

        /// @brief Uses stored create info to recreate vulkan image.
        virtual void Recreate();

        virtual void Create(const VkContext*         context,
                            VmaMemoryUsage           memoryUsage,
                            VmaAllocationCreateFlags flags,
                            VkExtent3D               extent,
                            VkImageUsageFlags        usage,
                            VkFormat                 format,
                            VkImageLayout            initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                            VkImageAspectFlags       aspectMask    = VK_IMAGE_ASPECT_COLOR_BIT,
                            std::string_view         name          = "UnnamedImage");

        /// @brief When doing a layout transition, specify the transition parameters.
        struct LayoutTransitionInfo
        {
            /// @brief New image layout is mandatory!
            VkImageLayout                NewImageLayout{VK_IMAGE_LAYOUT_UNDEFINED};
            std::optional<VkImageLayout> OldImageLayout{};
            VkAccessFlags                BarrierSrcAccessMask{0};
            VkAccessFlags                BarrierDstAccessMask{0};
            VkPipelineStageFlags         SrcStage{};
            VkPipelineStageFlags         DstStage{};
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
        virtual void TransitionLayout(VkImageLayout newLayout, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);
        virtual void TransitionLayout(ManagedImage::QuickTransition& quickTransition, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

        /// @brief Detailed layout transition.
        virtual void TransitionLayout(LayoutTransitionInfo& transitionInfo);

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
        FORAY_PROPERTY_CGET(ImageLayout)
        FORAY_PROPERTY_CGET(Allocation)
        FORAY_PROPERTY_CGET(AllocInfo)
        FORAY_PROPERTY_CGET(Format)
        FORAY_PROPERTY_CGET(Extent3D)
        FORAY_PROPERTY_GET(Name)
        FORAY_PROPERTY_CGET(Name)

        virtual ManagedImage& SetName(std::string_view name) override;

        VkSampleCountFlagBits GetSampleCount() { return mCreateInfo.ImageCI.samples; }

        /// @brief Set layout externally. Use with caution. Can sometimes be necessary if an image layout was changed on the GPU during
        /// a renderpass.
        void SetLayoutChanged(VkImageLayout layout) { mImageLayout = layout; }

      protected:
        const VkContext*  mContext{};
        CreateInfo        mCreateInfo;
        VkImage           mImage{};
        VkImageView       mImageView{};
        VkImageLayout     mImageLayout{};
        VkFormat          mFormat{};
        VmaAllocation     mAllocation{};
        VmaAllocationInfo mAllocInfo{};
        VkDeviceSize      mSize{};
        VkExtent3D        mExtent3D{};

        void CheckImageFormatSupport(CreateInfo& createInfo);
        void UpdateDebugNames();
    };
}  // namespace foray::core