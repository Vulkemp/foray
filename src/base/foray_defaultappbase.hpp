#pragma once
#include "../core/foray_samplercollection.hpp"
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

        FORAY_GETTER_MR(RenderLoop)
        FORAY_GETTER_MR(OsManager)
        FORAY_GETTER_MR(Instance)
        FORAY_GETTER_MR(Device)
        FORAY_GETTER_MR(WindowSwapchain)

        /// @brief Runs through the entire application lifetime
        int32_t Run();

      protected:
        /// @brief Called before any initialization happens.
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
        /// @brief Called after all DefaultAppBase members have been initialized, but before the renderloop starts. Initialize your application here
        inline virtual void ApiInit() {}
        /// @brief Called whenever the swapchain has been resized
        /// @param size New size of the swapchain
        inline virtual void ApiOnResized(VkExtent2D size) {}
        /// @brief Override this method to react to events
        inline virtual void ApiOnEvent(const osi::Event* event) {}
        /// @brief Called once every frame. Render your application here
        /// @param renderInfo Frame specific information
        /// @details Used command buffers must be fully recorded and submitted by the overrider. The swapchain image must be prepared for present In one of these command buffers!
        inline virtual void ApiRender(FrameRenderInfo& renderInfo) {}
        /// @brief Called back once the frame of the index has finished executing on the GPU, so QueryResults etc. are ready to be obtained
        /// @param frameIndex Index of the frame that has finished executing
        inline virtual void ApiFrameFinishedExecuting(uint64_t frameIndex) {}
        /// @brief Called whenever the shader compiler has detected a change and shaders have successfully been recompiled
        inline virtual void ApiOnShadersRecompiled() {}
        /// @brief Called after the application has been requested to shut down but before DefaultAppBase finalizes itself.
        inline virtual void ApiDestroy() {}

        /// @brief Call this with a renderstage for automatic OnResized/OnShadersRecompiled calls. Register order is maintained for callbacks.
        virtual void RegisterRenderStage(stages::RenderStage* stage);
        /// @brief Call this with a renderstage to unsubscribe from automatic calls
        virtual void UnregisterRenderStage(stages::RenderStage* stage);

        /// @brief [Internal] Initializes DefaultAppBase
        virtual void Init();
        /// @brief [Internal] Initializes Queue
        virtual void InitGetQueue();
        /// @brief [Internal] Initializes Command Pool
        virtual void InitCommandPool();
        /// @brief [Internal] Initializes VmaAllocator
        virtual void InitCreateVma();
        /// @brief [Internal] Initializes Synchronization Objects (InFlightFrame vector)
        virtual void InitSyncObjects();

        /// @brief [Internal] Recreates the swapchain
        virtual void RecreateSwapchain();
        /// @brief [Internal] Handler for swapchain resize
        inline void OnResized(VkExtent2D size);

        /// @brief [Internal] Polls and distributes events from the SDL subsystem
        virtual void PollEvents();

        /// @brief [Internal] Checks next frames InFlightFrame object for completed execution
        virtual bool CanRenderNextFrame();
        /// @brief [Internal] Image Acquire, Image Present
        virtual void Render(RenderLoop::RenderInfo& renderInfo);
        /// @brief [Internal] Shader recompile handler
        virtual void OnShadersRecompiled();

        /// @brief [Internal] Finalizer
        virtual void Destroy();

        RenderLoop              mRenderLoop;
        osi::OsManager          mOsManager;
        VulkanInstance          mInstance;
        VulkanDevice            mDevice;
        VulkanWindowSwapchain   mWindowSwapchain;
        core::SamplerCollection mSamplerCollection;
        core::Context           mContext;

        /// @brief Increase this in an early init method to get auxiliary command buffers
        uint32_t                                        mAuxiliaryCommandBufferCount = 0;
        std::array<InFlightFrame, INFLIGHT_FRAME_COUNT> mInFlightFrames;
        uint32_t                                        mInFlightFrameIndex = 0;
        uint64_t                                        mRenderedFrameCount = 0;

        std::vector<stages::RenderStage*> mRegisteredStages;

        fp64_t mLastShadersCheckedTimestamp = 0.0;
    };
}  // namespace foray::base