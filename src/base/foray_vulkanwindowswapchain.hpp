#pragma once
#include "../core/foray_swapchainimageinfo.hpp"
#include "../foray_vkb.hpp"
#include "../osi/foray_osi_declares.hpp"
#include "../osi/foray_window.hpp"
#include "foray_base_declares.hpp"
#include <functional>

namespace foray::base {
    /// @brief Combines Window, Surface and Swapchain into one managing object
    class VulkanWindowSwapchain
    {
      public:
        /// @brief Function called before window is created
        using BeforeWindowCreateFunctionPointer = std::function<void(osi::Window&)>;
        /// @brief Function called before the swapchain is created
        using BeforeSwapchainBuildFunctionPointer = std::function<void(vkb::SwapchainBuilder&)>;
        /// @brief Function called when a resize occurs
        using OnResizedFunctionPointer = std::function<void(VkExtent2D)>;
        /// @brief Function called for naming swapchain images.
        using MakeSwapchainImageNameFunctionPointer = std::function<std::string(uint32_t)>;

        VulkanWindowSwapchain() = default;
        inline VulkanWindowSwapchain(core::Context*                        context,
                                     BeforeWindowCreateFunctionPointer     beforeWindowCreateFunc,
                                     BeforeSwapchainBuildFunctionPointer   beforeSwapchainBuildFunc,
                                     OnResizedFunctionPointer              onResizedFunc,
                                     MakeSwapchainImageNameFunctionPointer makeSwapchainImageNameFunc)
            : mBeforeWindowCreateFunc{beforeWindowCreateFunc}
            , mBeforeSwapchainBuildFunc{beforeSwapchainBuildFunc}
            , mOnResizedFunc{onResizedFunc}
            , mMakeSwapchainImageNameFunc{makeSwapchainImageNameFunc}
            , mContext(context)
        {
        }

        FORAY_PROPERTY_R(Window)
        FORAY_PROPERTY_V(Surface)
        FORAY_PROPERTY_R(Swapchain)
        FORAY_PROPERTY_R(SwapchainImages)
        FORAY_PROPERTY_V(Context)

        /// @brief Create the Window
        void        CreateWindow();
        /// @brief Create the Swapchain
        void        CreateSwapchain();
        inline bool Exists() const { return !!mSwapchain.swapchain; }
        void        Destroy();

        /// @brief Listens for WindowSizeChanged events to preemptively resize the swapchain
        void HandleEvent(const osi::Event* event);
        /// @brief Call to recreate the swapchain
        void RecreateSwapchain();

      protected:
        void ExtractSwapchainImages();
        void DestroySwapchain();

        BeforeWindowCreateFunctionPointer     mBeforeWindowCreateFunc     = nullptr;
        BeforeSwapchainBuildFunctionPointer   mBeforeSwapchainBuildFunc   = nullptr;
        OnResizedFunctionPointer              mOnResizedFunc              = nullptr;
        MakeSwapchainImageNameFunctionPointer mMakeSwapchainImageNameFunc = nullptr;

        bool mNameSwapchainImages = true;

        core::Context* mContext = nullptr;

        osi::Window                           mWindow;
        VkSurfaceKHR                          mSurface = nullptr;
        vkb::Swapchain                        mSwapchain;
        std::vector<core::SwapchainImageInfo> mSwapchainImages;
    };
}  // namespace foray::base
