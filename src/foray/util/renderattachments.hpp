#pragma once
#include "../base/base_declares.hpp"
#include "../basics.hpp"
#include "../core/core_declares.hpp"
#include "../core/managedresource.hpp"
#include "../mem.hpp"
#include "../stages/stages_declares.hpp"
#include "../vulkan.hpp"
#include <span>

namespace foray::util {

    /// @brief Type maintaining refences of rasterized dynamic rendering attachments
    /// @details Features
    ///  - Configures VkRenderingAttachmentInfo objects
    ///  - Configures Memory Barriers
    ///  - Configures Image layout cache
    class RenderAttachments
    {
      public:
        enum class EAttachmentBindpoint
        {
            Color,
            Depth,
            Stencil
        };

        enum class EAttachmentSource
        {
            Undefined,
            Bare,
            ManagedImage,
            Swapchain
        };

        enum class EAttachmentLoadOp
        {
            /// @brief Existing content is discarded, clear with clearvalue
            Clear,
            /// @brief Existing content is loaded
            Load,
            /// @brief Existing content is discarded, non-written areas may now contain invalid data
            Discard
        };

        struct BareAttachment
        {
            VkImage     Image;
            VkImageView View;
            VkFormat    Format;
        };

        /// @brief Description of an attachment
        struct Attachment
        {
            /// @brief Bindpoint
            EAttachmentBindpoint Bindpoint;
            /// @brief Source. If undefined, attachment is invalid
            EAttachmentSource Source;
            /// @brief Only populated if Source == EAttachmentSource::Bare
            std::vector<BareAttachment> BareRefs;
            /// @brief Only populated if Source == EAttachmentSource::ManagedImage
            std::vector<core::ManagedImage*> Images;
            /// @brief Only populated if Source == EAttachmentSource::Swapchain
            base::VulkanWindowSwapchain* Swapchain = nullptr;
            /// @brief Image layout during rendering
            VkImageLayout Layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
            /// @brief Loading old data, clearing, doing nothing (discard)
            EAttachmentLoadOp LoadOp = EAttachmentLoadOp::Clear;
            /// @brief false: no data is written (dont_care) true: data is written (store)
            bool Store = true;
            /// @brief If load op clear, what value to clear with
            VkClearValue ClearValue = VkClearValue{};

            Attachment();
            Attachment(EAttachmentBindpoint bindpoint, std::span<BareAttachment>, VkImageLayout layout, EAttachmentLoadOp mode, bool store, VkClearValue clearValue = {});
            Attachment(EAttachmentBindpoint bindpoint, std::span<core::ManagedImage*>, VkImageLayout layout, EAttachmentLoadOp mode, bool store, VkClearValue clearValue = {});
            Attachment(EAttachmentBindpoint bindpoint, base::VulkanWindowSwapchain*, VkImageLayout layout, EAttachmentLoadOp mode, bool store, VkClearValue clearValue = {});

            /// @brief Get image from source member according to EAttachmentSource value
            /// @param resourceIdx For source members with multiple values, select value with "resourceIdx % count"
            VkImage GetSourceImage(uint32_t resourceIdx) const;
            /// @brief Get image view from source member according to EAttachmentSource value
            /// @param resourceIdx For source members with multiple values, select value with "resourceIdx % count"
            VkImageView GetSourceView(uint32_t resourceIdx) const;
            /// @brief Get the format of the underlying image according to EAttachmentSource value
            VkFormat GetSourceFormat() const;
            /// @brief Make an attachment info according to EAttachmentSource value
            /// @param resourceIdx For source members with multiple values, select value with "resourceIdx % count"
            VkRenderingAttachmentInfo GetAttachmentInfo(uint32_t resourceIdx) const;
            /// @brief Make a memory barrier according to EAttachmentSource value
            /// @param resourceIdx For source members with multiple values, select value with "resourceIdx % count"
            /// @param layoutCache Updates image layout cache with correct value
            VkImageMemoryBarrier2 MakeBarrier(uint32_t resourceIdx, core::ImageLayoutCache& layoutCache) const;
        };

#pragma region Set Color Output Attachment
        /// @brief Set an output attachment
        /// @param idx Id / fragment output location
        /// @param attachment attachment description
        RenderAttachments& SetAttachment(uint32_t idx, const Attachment& attachment);
#pragma region Cleared
        /// @brief Set an output attachment (cleared where not written to)
        /// @param idx Id / fragment output location
        /// @param image image to use as attachment
        /// @param layout layout of the image during rendering
        /// @param clearValue clear value to clear non-rendered areas with
        RenderAttachments& SetAttachmentCleared(uint32_t idx, core::ManagedImage* image, VkImageLayout layout, VkClearColorValue clearValue);
        /// @brief Set an output attachment (cleared where not written to)
        /// @param idx Id / fragment output location
        /// @param images images to use as attachments (selected via resourceIdx)
        /// @param layout layout of the image during rendering
        /// @param clearValue clear value to clear non-rendered areas with
        RenderAttachments& SetAttachmentCleared(uint32_t idx, std::span<core::ManagedImage*> images, VkImageLayout layout, VkClearColorValue clearValue);
        /// @brief Set an output attachment (cleared where not written to)
        /// @param idx Id / fragment output location
        /// @param image image to use as attachment
        /// @param layout layout of the image during rendering
        /// @param clearValue clear value to clear non-rendered areas with
        RenderAttachments& SetAttachmentCleared(uint32_t idx, BareAttachment image, VkImageLayout layout, VkClearColorValue clearValue);
        /// @brief Set an output attachment (cleared where not written to)
        /// @param idx Id / fragment output location
        /// @param images images to use as attachments (selected via resourceIdx)
        /// @param layout layout of the image during rendering
        /// @param clearValue clear value to clear non-rendered areas with
        RenderAttachments& SetAttachmentCleared(uint32_t idx, std::span<BareAttachment> images, VkImageLayout layout, VkClearColorValue clearValue);
        /// @brief Set an output attachment (cleared where not written to)
        /// @param idx Id / fragment output location
        /// @param swapchain images to use as attachments (selected via resourceIdx)
        /// @param layout layout of the image during rendering
        /// @param clearValue clear value to clear non-rendered areas with
        RenderAttachments& SetAttachmentCleared(uint32_t idx, base::VulkanWindowSwapchain* swapchain, VkImageLayout layout, VkClearColorValue clearValue);
#pragma endregion
#pragma region Discarded
        /// @brief Set an output attachment (discarded before write)
        /// @param idx Id / fragment output location
        /// @param image image to use as attachment
        /// @param layout layout of the image during rendering
        RenderAttachments& SetAttachmentDiscarded(uint32_t idx, core::ManagedImage* image, VkImageLayout layout);
        /// @brief Set an output attachment (discarded before write)
        /// @param idx Id / fragment output location
        /// @param images images to use as attachments (selected via resourceIdx)
        /// @param layout layout of the image during rendering
        RenderAttachments& SetAttachmentDiscarded(uint32_t idx, std::span<core::ManagedImage*> images, VkImageLayout layout);
        /// @brief Set an output attachment (discarded before write)
        /// @param idx Id / fragment output location
        /// @param image image to use as attachment
        /// @param layout layout of the image during rendering
        RenderAttachments& SetAttachmentDiscarded(uint32_t idx, BareAttachment image, VkImageLayout layout);
        /// @brief Set an output attachment (discarded before write)
        /// @param idx Id / fragment output location
        /// @param images images to use as attachments (selected via resourceIdx)
        /// @param layout layout of the image during rendering
        RenderAttachments& SetAttachmentDiscarded(uint32_t idx, std::span<BareAttachment> images, VkImageLayout layout);
        /// @brief Set an output attachment (discarded before write)
        /// @param idx Id / fragment output location
        /// @param swapchain images to use as attachments (selected via resourceIdx)
        /// @param layout layout of the image during rendering
        RenderAttachments& SetAttachmentDiscarded(uint32_t idx, base::VulkanWindowSwapchain* swapchain, VkImageLayout layout);
#pragma endregion
#pragma region Loaded
        /// @brief Set an output attachment (loaded where not written to)
        /// @param idx Id / fragment output location
        /// @param image image to use as attachment
        /// @param layout layout of the image during rendering
        RenderAttachments& SetAttachmentLoaded(uint32_t idx, core::ManagedImage* image, VkImageLayout layout);
        /// @brief Set an output attachment (loaded where not written to)
        /// @param idx Id / fragment output location
        /// @param images images to use as attachments (selected via resourceIdx)
        /// @param layout layout of the image during rendering
        RenderAttachments& SetAttachmentLoaded(uint32_t idx, std::span<core::ManagedImage*> images, VkImageLayout layout);
        /// @brief Set an output attachment (loaded where not written to)
        /// @param idx Id / fragment output location
        /// @param image image to use as attachment
        /// @param layout layout of the image during rendering
        RenderAttachments& SetAttachmentLoaded(uint32_t idx, BareAttachment image, VkImageLayout layout);
        /// @brief Set an output attachment (loaded where not written to)
        /// @param idx Id / fragment output location
        /// @param images images to use as attachments (selected via resourceIdx)
        /// @param layout layout of the image during rendering
        RenderAttachments& SetAttachmentLoaded(uint32_t idx, std::span<BareAttachment> images, VkImageLayout layout);
        /// @brief Set an output attachment (loaded where not written to)
        /// @param idx Id / fragment output location
        /// @param swapchain images to use as attachments (selected via resourceIdx)
        /// @param layout layout of the image during rendering
        RenderAttachments& SetAttachmentLoaded(uint32_t idx, base::VulkanWindowSwapchain* swapchain, VkImageLayout layout);
#pragma endregion
#pragma endregion
#pragma region Add Color Output Attachment
        /// @brief Add an output attachment to the back of the output attachment list
        /// @param attachment attachment description
        RenderAttachments& AddAttachment(const Attachment& attachment);
#pragma region Cleared
        /// @brief Set an output attachment (cleared where not written to)
        /// @param image image to use as attachment
        /// @param layout layout of the image during rendering
        /// @param clearValue clear value to clear non-rendered areas with
        RenderAttachments& AddAttachmentCleared(core::ManagedImage* image, VkImageLayout layout, VkClearColorValue clearValue);
        /// @brief Set an output attachment (cleared where not written to)
        /// @param images images to use as attachments (selected via resourceIdx)
        /// @param layout layout of the image during rendering
        /// @param clearValue clear value to clear non-rendered areas with
        RenderAttachments& AddAttachmentCleared(std::span<core::ManagedImage*> images, VkImageLayout layout, VkClearColorValue clearValue);
        /// @brief Set an output attachment (cleared where not written to)
        /// @param image image to use as attachment
        /// @param layout layout of the image during rendering
        /// @param clearValue clear value to clear non-rendered areas with
        RenderAttachments& AddAttachmentCleared(BareAttachment image, VkImageLayout layout, VkClearColorValue clearValue);
        /// @brief Set an output attachment (cleared where not written to)
        /// @param images images to use as attachments (selected via resourceIdx)
        /// @param layout layout of the image during rendering
        /// @param clearValue clear value to clear non-rendered areas with
        RenderAttachments& AddAttachmentCleared(std::span<BareAttachment> images, VkImageLayout layout, VkClearColorValue clearValue);
        /// @brief Set an output attachment (cleared where not written to)
        /// @param swapchain images to use as attachments (selected via resourceIdx)
        /// @param layout layout of the image during rendering
        /// @param clearValue clear value to clear non-rendered areas with
        RenderAttachments& AddAttachmentCleared(base::VulkanWindowSwapchain* swapchain, VkImageLayout layout, VkClearColorValue clearValue);
#pragma endregion
#pragma region Discarded
        RenderAttachments& AddAttachmentDiscarded(core::ManagedImage* image, VkImageLayout layout);
        RenderAttachments& AddAttachmentDiscarded(std::span<core::ManagedImage*> images, VkImageLayout layout);
        RenderAttachments& AddAttachmentDiscarded(BareAttachment image, VkImageLayout layout);
        RenderAttachments& AddAttachmentDiscarded(std::span<BareAttachment> images, VkImageLayout layout);
        RenderAttachments& AddAttachmentDiscarded(base::VulkanWindowSwapchain* swapchain, VkImageLayout layout);
#pragma endregion
#pragma region Loaded
        RenderAttachments& AddAttachmentLoaded(core::ManagedImage* image, VkImageLayout layout);
        RenderAttachments& AddAttachmentLoaded(std::span<core::ManagedImage*> images, VkImageLayout layout);
        RenderAttachments& AddAttachmentLoaded(BareAttachment image, VkImageLayout layout);
        RenderAttachments& AddAttachmentLoaded(std::span<BareAttachment> images, VkImageLayout layout);
        RenderAttachments& AddAttachmentLoaded(base::VulkanWindowSwapchain* swapchain, VkImageLayout layout);
#pragma endregion
#pragma endregion
#pragma region Set Depth Attachment
        RenderAttachments& SetDepthAttachmentCleared(core::ManagedImage* image, VkImageLayout layout, VkClearDepthStencilValue clearValue, bool store = true);
        RenderAttachments& SetDepthAttachmentCleared(BareAttachment image, VkImageLayout layout, VkClearDepthStencilValue clearValue, bool store = true);
        RenderAttachments& SetDepthAttachmentLoaded(core::ManagedImage* image, VkImageLayout layout, bool store = true);
        RenderAttachments& SetDepthAttachmentLoaded(BareAttachment image, VkImageLayout layout, bool store = true);
#pragma endregion
#pragma region Set Stencil Attachment
        RenderAttachments& SetStencilAttachmentCleared(core::ManagedImage* image, VkImageLayout layout, VkClearDepthStencilValue clearValue, bool store = true);
        RenderAttachments& SetStencilAttachmentCleared(BareAttachment image, VkImageLayout layout, VkClearDepthStencilValue clearValue, bool store = true);
        RenderAttachments& SetStencilAttachmentLoaded(core::ManagedImage* image, VkImageLayout layout, bool store = true);
        RenderAttachments& SetStencilAttachmentLoaded(BareAttachment image, VkImageLayout layout, bool store = true);
#pragma endregion

        FORAY_PROPERTY_R(Attachments)
        FORAY_PROPERTY_V(Flags)
        FORAY_PROPERTY_V(ViewMask)

        void CmdBeginRendering(VkCommandBuffer cmdBuffer, VkExtent2D extent, core::ImageLayoutCache& layoutCache, uint32_t resourceIdx = 0) const;

        VkPipelineRenderingCreateInfo MakePipelineRenderingCi();

      protected:
        std::vector<Attachment> mAttachments;
        Local<Attachment>       mDepthAttachment;
        Local<Attachment>       mStencilAttachment;
        VkRenderingFlags        mFlags      = 0;
        uint32_t                mLayerCount = 1;
        uint32_t                mViewMask   = 0;
        void*                   mPNext      = nullptr;
        std::vector<VkFormat>   mAttachmentFormats;
    };
}  // namespace foray::util