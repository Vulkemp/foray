#pragma once
#include "../hsk_basics.hpp"
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include "../base/hsk_vkcontext.hpp"

namespace hsk {
    class IntermediateImage : public NoMoveDefaults
    {
      public:
        inline IntermediateImage() {}
        inline ~IntermediateImage() { Destroy(); }

        struct CreateInfo
        {
            VkImageCreateInfo       ImageCI{};
            VmaAllocationCreateInfo AllocCI{};

            CreateInfo();
        };

        inline virtual void Init(const VkContext* context, const CreateInfo& createInfo)
        {
            mContext = context;
            Init(createInfo);
        }
        virtual void Init(const CreateInfo& createInfo);


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

        void WriteDeviceLocalData(void* data, size_t size);

        virtual void Destroy();

        HSK_PROPERTY_CGET(Image)
        HSK_PROPERTY_CGET(ImageView)
        HSK_PROPERTY_CGET(ImageLayout)
        HSK_PROPERTY_CGET(Allocation)
        HSK_PROPERTY_CGET(AllocInfo)
        HSK_PROPERTY_CGET(Format)

      protected:
        const VkContext*  mContext{};
        VkImage           mImage{};
        VkImageView       mImageView{};
        VkImageLayout     mImageLayout{};
        VkFormat          mFormat{};
        VmaAllocation     mAllocation{};
        VmaAllocationInfo mAllocInfo{};
    };
}  // namespace hsk