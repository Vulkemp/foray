#pragma once
#include "../core/foray_swapchainimageinfo.hpp"
#include "../foray_vkb.hpp"
#include "../osi/foray_osi_declares.hpp"
#include "../osi/foray_window.hpp"
#include "foray_base_declares.hpp"
#include <functional>

namespace foray::base {
    class VulkanWindowSwapchain
    {
      public:
        using BeforeWindowCreateFunctionPointer     = std::function<void(Window&)>;
        using BeforeSwapchainBuildFunctionPointer   = std::function<void(vkb::SwapchainBuilder&)>;
        using OnResizedFunctionPointer              = std::function<void(VkExtent2D)>;
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

        FORAY_PROPERTY_ALLGET(Window)
        FORAY_PROPERTY_ALLGET(Surface)
        FORAY_PROPERTY_ALLGET(Swapchain)
        FORAY_PROPERTY_ALLGET(SwapchainImages)
        FORAY_PROPERTY_ALL(Context)

        void        CreateWindow();
        void        CreateSwapchain();
        inline bool Exists() const { return !!mSwapchain.swapchain; }
        void        Destroy();

        /// @brief Listens for WindowSizeChanged events to preemptively resize the swapchain
        /// @param event
        void HandleEvent(const Event* event);
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

        Window                                mWindow;
        VkSurfaceKHR                          mSurface = nullptr;
        vkb::Swapchain                        mSwapchain;
        std::vector<core::SwapchainImageInfo> mSwapchainImages;
    };
}  // namespace foray::base
