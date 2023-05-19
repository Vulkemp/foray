#pragma once
#include "../core/swapchainimageinfo.hpp"
#include "../vkb.hpp"
#include "../osi/osi_declares.hpp"
#include "../osi/window.hpp"
#include "../stages/renderdomain.hpp"
#include "base_declares.hpp"
#include <functional>

namespace foray::base {
    /// @brief Combines Window, Surface and Swapchain into one managing object
    class VulkanWindowSwapchain : public stages::RenderDomain
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

        VulkanWindowSwapchain() : stages::RenderDomain() {}
        inline VulkanWindowSwapchain(core::Context*                        context,
                                     BeforeWindowCreateFunctionPointer     beforeWindowCreateFunc,
                                     BeforeSwapchainBuildFunctionPointer   beforeSwapchainBuildFunc,
                                     OnResizedFunctionPointer              onResizedFunc,
                                     MakeSwapchainImageNameFunctionPointer makeSwapchainImageNameFunc)
            : stages::RenderDomain()
            , mBeforeWindowCreateFunc{beforeWindowCreateFunc}
            , mBeforeSwapchainBuildFunc{beforeSwapchainBuildFunc}
            , mOnResizedFunc{onResizedFunc}
            , mMakeSwapchainImageNameFunc{makeSwapchainImageNameFunc}
            , mContext(context)
        {
        }

        FORAY_PROPERTY_R(Window)
        FORAY_PROPERTY_R(Swapchain)
        FORAY_PROPERTY_R(SwapchainImages)
        FORAY_PROPERTY_V(Context)

        /// @brief Create the Window
        void CreateWindow();
        /// @brief Create the Swapchain
        void        CreateSwapchain();
        inline bool Exists() const { return !!mSwapchain.swapchain; }

        virtual ~VulkanWindowSwapchain();

        VkSurfaceKHR GetOrCreateSurface();

        /// @brief Listens for WindowSizeChanged events to preemptively resize the swapchain
        virtual void OnWindowResized(const osi::EventWindowResized* message);
        /// @brief Call to recreate the swapchain
        void RecreateSwapchain();

        inline operator VkSwapchainKHR() { return mSwapchain.swapchain; }

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
