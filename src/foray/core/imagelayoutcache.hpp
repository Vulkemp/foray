#pragma once
#include "../vulkan.hpp"
#include "core_declares.hpp"
#include <unordered_map>

namespace foray::core {
    /// @brief Tracks ImageLayouts over the course of a frame rendering process
    /// @details
    /// - Why is the use of this necessary: If passing VkImageLayout::UNDEFINED as the old image layout in a layout transition, the driver is free to decide wether it wants to transition the old data or simply discard it instead.
    /// - Individual array layers and mip map levels can be in differing layouts, this requires manual setup, as this class only tracks layout of a complete image
    /// - The FrameRenderInfo's Image Layout Cache is a new object every frame. Frame Buffers from the previous frame which need to be read in the current frame must be manually set in the new object.
    class ImageLayoutCache
    {
      public:
        /// @brief Get the currently cached layout of image identified by imageName
        VkImageLayout Get(VkImage image) const;
        /// @brief Get the currently cached layout of image
        VkImageLayout Get(const ManagedImage& image) const;
        /// @brief Get the currently cached layout of image
        VkImageLayout Get(const ManagedImage* image) const;

        /// @brief Set the cached layout of image
        void Set(VkImage image, VkImageLayout layout);
        /// @brief Set the cached layout of image
        void Set(const ManagedImage& image, VkImageLayout layout);
        /// @brief Set the cached layout of image
        void Set(const ManagedImage* image, VkImageLayout layout);

        /// @brief See VkImageMemoryBarrier
        struct Barrier
        {
            VkAccessFlags           SrcAccessMask       = VkAccessFlagBits::VK_ACCESS_NONE;
            VkAccessFlags           DstAccessMask       = VkAccessFlagBits::VK_ACCESS_NONE;
            uint32_t                SrcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            uint32_t                DstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            VkImageLayout           NewLayout;
            VkImageSubresourceRange SubresourceRange = VkImageSubresourceRange{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1U, .layerCount = 1U};
        };

        /// @brief See VkImageMemoryBarrier2
        struct Barrier2
        {
            VkPipelineStageFlags2   SrcStageMask        = VK_PIPELINE_STAGE_2_NONE;
            VkAccessFlags2          SrcAccessMask       = VK_ACCESS_2_NONE;
            VkPipelineStageFlags2   DstStageMask        = VK_PIPELINE_STAGE_2_NONE;
            VkAccessFlags2          DstAccessMask       = VK_ACCESS_2_NONE;
            uint32_t                SrcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            uint32_t                DstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            VkImageLayout           NewLayout;
            VkImageSubresourceRange SubresourceRange = VkImageSubresourceRange{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1U, .layerCount = 1U};
        };

        /// @brief Constructs a VkImageMemoryBarrier struct and updates the stored layout
        /// @param name Name of the image
        /// @param image Image
        /// @param barrier Layout Transition Barrier information
        VkImageMemoryBarrier  MakeBarrier(VkImage image, const Barrier& barrier);
        /// @brief Constructs a VkImageMemoryBarrier struct and updates the stored layout
        /// @param image Image
        /// @param barrier Layout Transition Barrier information
        VkImageMemoryBarrier  MakeBarrier(const ManagedImage* image, const Barrier& barrier);
        /// @brief Constructs a VkImageMemoryBarrier struct and updates the stored layout
        /// @param image Image
        /// @param barrier Layout Transition Barrier information
        VkImageMemoryBarrier  MakeBarrier(const ManagedImage& image, const Barrier& barrier);
        /// @brief Constructs a VkImageMemoryBarrier2 struct and updates the stored layout
        /// @param name Name of the image
        /// @param image Image
        /// @param barrier Layout Transition Barrier2 information
        VkImageMemoryBarrier2 MakeBarrier(VkImage image, const Barrier2& barrier);
        /// @brief Constructs a VkImageMemoryBarrier2 struct and updates the stored layout
        /// @param image Image
        /// @param barrier Layout Transition Barrier2 information
        VkImageMemoryBarrier2 MakeBarrier(const ManagedImage* image, const Barrier2& barrier);
        /// @brief Constructs a VkImageMemoryBarrier2 struct and updates the stored layout
        /// @param image Image
        /// @param barrier Layout Transition Barrier2 information
        VkImageMemoryBarrier2 MakeBarrier(const ManagedImage& image, const Barrier2& barrier);

        /// @brief Writes a dedicated vkCmdPipelineBarrier command
        /// @param cmdBuffer Command Buffer
        /// @param name Name of the image
        /// @param image Image
        /// @param barrier Layout Transition Barrier information
        /// @param srcStageMask Source Stage Mask
        /// @param dstStageMask Dest Stage Mask
        /// @param depFlags DependencyFlags
        void CmdBarrier(VkCommandBuffer      cmdBuffer,
                        VkImage              image,
                        const Barrier&       barrier,
                        VkPipelineStageFlags srcStageMask,
                        VkPipelineStageFlags dstStageMask,
                        VkDependencyFlags    depFlags = 0);
        /// @brief Writes a dedicated vkCmdPipelineBarrier command
        /// @param cmdBuffer Command Buffer
        /// @param image Image
        /// @param barrier Layout Transition Barrier information
        /// @param srcStageMask Source Stage Mask
        /// @param dstStageMask Dest Stage Mask
        /// @param depFlags DependencyFlags
        void CmdBarrier(VkCommandBuffer      cmdBuffer,
                        const ManagedImage*  image,
                        const Barrier&       barrier,
                        VkPipelineStageFlags srcStageMask,
                        VkPipelineStageFlags dstStageMask,
                        VkDependencyFlags    depFlags = 0);
        /// @brief Writes a dedicated vkCmdPipelineBarrier command
        /// @param cmdBuffer Command Buffer
        /// @param image Image
        /// @param barrier Layout Transition Barrier information
        /// @param srcStageMask Source Stage Mask
        /// @param dstStageMask Dest Stage Mask
        /// @param depFlags DependencyFlags
        void CmdBarrier(VkCommandBuffer      cmdBuffer,
                        const ManagedImage&  image,
                        const Barrier&       barrier,
                        VkPipelineStageFlags srcStageMask,
                        VkPipelineStageFlags dstStageMask,
                        VkDependencyFlags    depFlags = 0);
        /// @brief Writes a dedicated vkCmdPipelineBarrier2 command
        /// @param cmdBuffer Command Buffer
        /// @param name Name of the image
        /// @param image Image
        /// @param barrier Layout Transition Barrier2 information
        /// @param depFlags DependencyFlags
        void CmdBarrier(VkCommandBuffer cmdBuffer, VkImage image, const Barrier2& barrier, VkDependencyFlags depFlags = 0);
        /// @brief Writes a dedicated vkCmdPipelineBarrier2 command
        /// @param cmdBuffer Command Buffer
        /// @param image Image
        /// @param barrier Layout Transition Barrier2 information
        /// @param depFlags DependencyFlags
        void CmdBarrier(VkCommandBuffer cmdBuffer, const ManagedImage* image, const Barrier2& barrier, VkDependencyFlags depFlags = 0);
        /// @brief Writes a dedicated vkCmdPipelineBarrier2 command
        /// @param cmdBuffer Command Buffer
        /// @param image Image
        /// @param barrier Layout Transition Barrier2 information
        /// @param depFlags DependencyFlags
        void CmdBarrier(VkCommandBuffer cmdBuffer, const ManagedImage& image, const Barrier2& barrier, VkDependencyFlags depFlags = 0);

      protected:
        std::unordered_map<VkImage, VkImageLayout> mLayoutCache;
    };
}  // namespace foray::core
