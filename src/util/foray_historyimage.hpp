#pragma once
#include "../base/foray_base_declares.hpp"
#include "../core/foray_managedimage.hpp"

namespace foray::util {
    class HistoryImage
    {
      public:
        void Create(core::Context* context, core::ManagedImage* source);

        void Resize(const VkExtent2D& size);

        void CmdCopySourceToHistory(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo);

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
