#pragma once
#include "../hsk_basics.hpp"
#include <vulkan/vulkan.h>

namespace hsk {
    class FrameUpdateInfo
    {
      public:
        /// @brief Frame Delay in Seconds
        HSK_PROPERTY_ALL(FrameTime)
        HSK_PROPERTY_ALL(FrameNumber)

        FrameUpdateInfo() {}

      protected:
        double mFrameTime = 0;
        /// @brief Number of complete frames rendered since application startup
        uint64_t mFrameNumber = 0;
    };
    
    class FrameRenderInfo : public FrameUpdateInfo
    {
      public:
        HSK_PROPERTY_ALL(FrameObjectsIndex)
        HSK_PROPERTY_ALL(CommandBuffer)

        FrameRenderInfo() : FrameUpdateInfo() {}

      protected:
        /// @brief Index of in-flight frame synchronisation objects this frame is using
        uint32_t mFrameObjectsIndex = 0;
        /// @brief Render command buffer (do not initialize, reset or finalize it!)
        VkCommandBuffer mCommandBuffer = nullptr;
    };
}  // namespace hsk