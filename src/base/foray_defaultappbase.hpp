#pragma once
#include "../foray_vma.hpp"
#include "../osi/foray_osmanager.hpp"
#include "../stages/foray_stages_declares.hpp"
#include "foray_framerenderinfo.hpp"
#include "foray_renderloop.hpp"
#include "foray_vulkandevice.hpp"
#include "foray_vulkaninstance.hpp"
#include "foray_vulkanwindowswapchain.hpp"
#include <array>
#include <vector>

namespace foray::base {

    /// @brief Intended as base class for demo applications. Compared to MinimalAppBase it offers a complete simple vulkan setup.
    class DefaultAppBase
    {
      public:
        DefaultAppBase();
        virtual ~DefaultAppBase() = default;

        FORAY_PROPERTY_ALLGET(RenderLoop)
        FORAY_PROPERTY_ALLGET(OsManager)
        FORAY_PROPERTY_ALLGET(Instance)
        FORAY_PROPERTY_ALLGET(Device)
        FORAY_PROPERTY_ALLGET(WindowSwapchain)
        FORAY_PROPERTY_SET(Context)

        int32_t Run();

      protected:
        inline virtual void ApiBeforeInit() {}

        /// @brief Override this method to alter vulkan instance creation parameters via the instance builder
        inline virtual void ApiBeforeWindowCreate(osi::Window& window) {}
        /// @brief Override this method to alter vulkan instance creation parameters via the instance builder
        inline virtual void ApiBeforeInstanceCreate(vkb::InstanceBuilder& instanceBuilder) {}
        /// @brief Alter physical device selection.
        inline virtual void ApiBeforeDeviceSelection(vkb::PhysicalDeviceSelector& pds) {}
        /// @brief Alter device selection.
        inline virtual void ApiBeforeDeviceBuilding(vkb::DeviceBuilder& deviceBuilder) {}
        /// @brief Before building the swapchain
        inline virtual void ApiBeforeSwapchainBuilding(vkb::SwapchainBuilder& swapchainBuilder) {}
        inline virtual void ApiInit() {}
        inline virtual void ApiOnResized(VkExtent2D size) {}
        /// @brief Override this method to react to events
        inline virtual void ApiOnEvent(const osi::Event* event) {}
        inline virtual void ApiRender(FrameRenderInfo& renderInfo) {}
        inline virtual void ApiQueryResultsAvailable(uint64_t frameIndex) {}
        inline virtual void ApiOnShadersRecompiled() {}
        inline virtual void ApiDestroy() {}

        virtual void Init();
        virtual void InitGetQueue();
        virtual void InitCommandPool();
        virtual void InitCreateVma();
        virtual void InitSyncObjects();

        virtual void RecreateSwapchain();
        inline void  OnResized(VkExtent2D size);

        /// @brief Polls and distributes events from the SDL subsystem
        virtual void PollEvents();

        virtual bool CanRenderNextFrame();
        virtual void Render(RenderLoop::RenderInfo& renderInfo);
        virtual void OnShadersRecompiled();

        virtual void Destroy();

        virtual void RegisterRenderStage(stages::RenderStage* stage);
        virtual void UnregisterRenderStage(stages::RenderStage* stage);

        RenderLoop            mRenderLoop;
        osi::OsManager        mOsManager;
        VulkanInstance        mInstance;
        VulkanDevice          mDevice;
        VulkanWindowSwapchain mWindowSwapchain;
        core::Context         mContext;


        /// @brief The applications vulkan context.
        uint32_t mRequiredVulkanApiVersion = VK_API_VERSION_1_3;

        uint32_t                                        mAuxiliaryCommandBufferCount = 0;
        std::array<InFlightFrame, INFLIGHT_FRAME_COUNT> mInFlightFrames;
        uint32_t                                        mInFlightFrameIndex = 0;
        uint64_t                                        mRenderedFrameCount = 0;

        std::vector<stages::RenderStage*> mRegisteredStages;

        fp64_t mLastShadersCheckedTimestamp = 0.0;
    };
}  // namespace foray::base