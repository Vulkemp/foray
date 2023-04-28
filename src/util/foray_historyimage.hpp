#pragma once
#include "../base/foray_base_declares.hpp"
#include "../core/foray_managedimage.hpp"
#include "../foray_mem.hpp"

namespace foray::util {
    /// @brief Helper object managing a copy of an existing image for use as history information (reprojection etc.)
    /// @details
    /// The history image is named "History.{source->GetName()}", and inherits the entire createinfo of the original
    class HistoryImage
    {
      public:
        /// @param context Required to initiate own copy
        /// @param source Source image to copy
        /// @param additionalUsageFlags UsageFlags to apply to the createinfo for the copy
        HistoryImage(core::Context* context, core::ManagedImage* source, VkImageUsageFlags additionalUsageFlags = 0U);

        virtual ~HistoryImage();

        void Resize(const VkExtent2D& size);

        void ApplyToLayoutCache(core::ImageLayoutCache& layoutCache);

        /// @brief Pipeline barriers, cmdCopyImage from source -> history
        void CmdCopySourceToHistory(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo);

        /// @brief Batch record pipeline barriers and cmdCopyImage for a set of history images
        static void sMultiCopySourceToHistory(const std::vector<HistoryImage*>& historyImages, VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo);

        inline core::ManagedImage* GetHistoryImage() { return mHistory.Get(); }


        FORAY_PROPERTY_V(HistoricLayout)

      protected:
        core::ManagedImage*       mSource = nullptr;
        Local<core::ManagedImage> mHistory;
        VkImageLayout             mHistoricLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    };
}  // namespace foray::util
