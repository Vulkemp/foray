#pragma once
#include "../base/base_declares.hpp"
#include "../core/image.hpp"
#include "../mem.hpp"

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
        HistoryImage(core::Context* context, core::Image* source, VkImageUsageFlags additionalUsageFlags = 0U);

        virtual ~HistoryImage() = default;

        void Resize(const VkExtent2D& size);

        void ApplyToLayoutCache(core::ImageLayoutCache& layoutCache);

        /// @brief Pipeline barriers, cmdCopyImage from source -> history
        void CmdCopySourceToHistory(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo);

        /// @brief Batch record pipeline barriers and cmdCopyImage for a set of history images
        static void sMultiCopySourceToHistory(const std::vector<HistoryImage*>& historyImages, VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo);

        FORAY_GETTER_MEM(History)

        FORAY_PROPERTY_V(HistoricLayout)

      protected:
        core::Image*      mSource = nullptr;
        core::Local_Image mHistory;
        vk::ImageLayout            mHistoricLayout = vk::ImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    };
}  // namespace foray::util
