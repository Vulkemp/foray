#pragma once
#include "../base/foray_base_declares.hpp"
#include "../core/foray_managedimage.hpp"

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
        void Create(core::Context* context, core::ManagedImage* source, VkImageUsageFlags additionalUsageFlags = 0U);

        void Resize(const VkExtent2D& size);

        /// @brief Pipeline barriers, cmdCopyImage from source -> history
        void CmdCopySourceToHistory(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo);

        /// @brief Batch record pipeline barriers and cmdCopyImage for a set of history images
        static void sMultiCopySourceToHistory(const std::vector<HistoryImage*>& historyImages, VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo);

        inline bool Exists() const { return mHistory.Exists(); }

        void Destroy();

        inline core::ManagedImage& GetHistoryImage() { return mHistory; }

        inline operator core::ManagedImage&() { return mHistory; }
        inline operator const core::ManagedImage&() const { return mHistory; }

      protected:
        core::ManagedImage* mSource = nullptr;
        core::ManagedImage  mHistory;
    };
}  // namespace foray::util
