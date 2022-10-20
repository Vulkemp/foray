#pragma once
#include "../core/foray_vkcontext.hpp"
#include "../foray_vma.hpp"
#include "../osi/foray_window.hpp"
#include "../stages/foray_stages_declares.hpp"
#include "foray_framerenderinfo.hpp"
#include "foray_minimalappbase.hpp"
#include "foray_renderloop.hpp"
#include "foray_vulkandevice.hpp"
#include "foray_vulkaninstance.hpp"
#include "foray_vulkanwindow.hpp"
#include <unordered_set>

namespace foray::base {

    /// @brief Intended as base class for demo applications. Compared to MinimalAppBase it offers a complete simple vulkan setup.
    class DefaultAppBase
    {
      public:
        DefaultAppBase();
        virtual ~DefaultAppBase() = default;

      protected:
        inline virtual void ApiBeforeInit() {}

        /// @brief Override this method to alter vulkan instance creation parameters via the instance builder
        inline virtual void ApiBeforeInstanceCreate(vkb::InstanceBuilder& instanceBuilder) {}
        /// @brief Alter physical device selection.
        inline virtual void ApiBeforeDeviceSelection(vkb::PhysicalDeviceSelector& pds) {}
        /// @brief Alter device selection.
        inline virtual void ApiBeforeDeviceBuilding(vkb::DeviceBuilder& deviceBuilder) {}
        /// @brief Before building the swapchain
        inline virtual void ApiBeforeSwapchainBuilding(vkb::SwapchainBuilder& swapchainBuilder) {}
        inline virtual void ApiOnResized(VkExtent2D size) {}
        inline virtual void ApiRender(FrameRenderInfo& renderInfo) {}
        inline virtual void ApiQueryResultsAvailable(uint64_t frameIndex) {}
        inline virtual void ApiOnShadersRecompiled() {}
        inline virtual void ApiDestroy() {}

        virtual void Init();
        void         BeforeInstanceCreate(vkb::InstanceBuilder& instanceBuilder);
        virtual void InitGetVkQueues();
        virtual void InitCommandPool();
        virtual void InitCreateVma();
        virtual void InitSyncObjects();

        virtual void RecreateSwapchain();

        virtual bool CanRenderNextFrame();
        virtual void Render(float delta);

        virtual void Destroy();

        virtual void RegisterRenderStage(stages::RenderStage* stage);
        virtual void UnregisterRenderStage(stages::RenderStage* stage);

        RenderLoop     mRenderLoop;
        OsManager      mOsManager;
        VulkanInstance mInstance;
        VulkanDevice   mDevice;
        VulkanWindow   mWindow;

        /// @brief The applications vulkan context.
        core::VkContext mContext;
        uint32_t        mRequiredVulkanApiVersion = VK_API_VERSION_1_3;

        uint32_t                                        mAuxiliaryCommandBufferCount = 0;
        std::array<InFlightFrame, INFLIGHT_FRAME_COUNT> mInFlightFrames;
        uint32_t                                        mInFlightFrameIndex = 0;

        /// @brief Commandpool for the default queue.
        VkCommandPool mCommandPool{};

        std::unordered_set<stages::RenderStage*> mRegisteredStages;
    };
}  // namespace foray::base