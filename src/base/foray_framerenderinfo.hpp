#pragma once
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"

namespace foray::base {
    class FrameUpdateInfo
    {
      public:
        /// @brief Frame Delay in Seconds
        FORAY_PROPERTY_ALL(FrameTime)
        FORAY_PROPERTY_ALL(FrameNumber)
        FORAY_PROPERTY_ALL(CommandBuffer)

        FrameUpdateInfo() {}

      protected:
        double mFrameTime = 0;
        /// @brief Number of complete frames rendered since application startup
        uint64_t mFrameNumber = 0;
        /// @brief Render command buffer (do not initialize, reset or finalize it!)
        VkCommandBuffer mCommandBuffer = nullptr;
    };

    class FrameRenderInfo : public FrameUpdateInfo
    {
      public:
        FORAY_PROPERTY_ALL(FrameObjectsIndex)
        FORAY_PROPERTY_ALL(SwapchainImageIndex)

        FrameRenderInfo() : FrameUpdateInfo() {}

      protected:
        /// @brief Index of in-flight frame synchronisation objects this frame is using
        uint32_t mFrameObjectsIndex   = 0;
        uint32_t mSwapchainImageIndex = 0;
    };
}  // namespace foray::base