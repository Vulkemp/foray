#pragma once
#include "../hsk_basics.hpp"

namespace hsk {
    class FrameRenderInfo
    {
      public:
        /// @brief Frame Delay in Seconds
        HSK_PROPERTY_ALL(FrameTime)
        HSK_PROPERTY_ALL(FrameNumber)
        HSK_PROPERTY_ALL(FrameObjectsIndex)
        HSK_PROPERTY_ALL(CommandBuffer)

        FrameRenderInfo(VkImage& primaryOutput, VkImage& comparisonOutput) : mPrimaryOutputImage(primaryOutput), mComparisonOutputImage(comparisonOutput) {}

        FrameRenderInfo& SetPrimaryOutput(VkImage image) { mPrimaryOutputImage = image; return *this; }
        FrameRenderInfo& SetComparisonOutput(VkImage image) { mComparisonOutputImage = image; return *this; }

      protected:
        double mFrameTime = 0;
        /// @brief Number of complete frames rendered since application startup
        uint64_t mFrameNumber = 0;
        /// @brief Index of in-flight frame synchronisation objects this frame is using
        uint32_t mFrameObjectsIndex = 0;
        /// @brief Render command buffer (do not initialize, reset or finalize it!)
        VkCommandBuffer mCommandBuffer = nullptr;

        VkImage& mPrimaryOutputImage;
        VkImage& mComparisonOutputImage;
    };
}  // namespace hsk