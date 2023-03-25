#pragma once
#include "../bench/foray_hostbenchmark.hpp"
#include "../core/foray_samplercollection.hpp"
#include "../core/foray_shadermanager.hpp"
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
        FORAY_GETTER_MR(HostFrameRecordBenchmark)

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
        inline virtual void ApiOnSwapchainResized(VkExtent2D size) {}
        /// @brief Override this method to react to events
        inline virtual void ApiOnOsEvent(const osi::Event* event) {}
        /// @brief Called once every frame. Render your application here
        /// @param renderInfo Frame specific information
        /// @details Used command buffers must be fully recorded and submitted by the overrider. The swapchain image must be prepared for present In one of these command buffers!
        inline virtual void ApiRender(FrameRenderInfo& renderInfo) {}
        /// @brief Called back once the frame of the index has finished executing on the GPU, so QueryResults etc. are ready to be obtained
        /// @param frameIndex Index of the frame that has finished executing
        inline virtual void ApiFrameFinishedExecuting(uint64_t frameIndex) {}
        /// @brief Called after the application has been requested to shut down but before DefaultAppBase finalizes itself.
        inline virtual void ApiDestroy() {}

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

        /// @brief [Internal] Polls and distributes events from the SDL subsystem
        virtual void PollEvents();

        /// @brief [Internal] Checks next frames InFlightFrame object for completed execution
        virtual bool CanRenderNextFrame();
        /// @brief [Internal] Image Acquire, Image Present
        virtual void Render(RenderLoop::RenderInfo& renderInfo);
        /// @brief [Internal] Shader recompile handler
        virtual void OnOsEvent(const osi::Event* event);

        /// @brief [Internal] Finalizer
        virtual void Destroy();

        RenderLoop              mRenderLoop;
        osi::OsManager          mOsManager;
        VulkanInstance          mInstance;
        VulkanDevice            mDevice;
        VulkanWindowSwapchain   mWindowSwapchain;
        core::SamplerCollection mSamplerCollection;
        core::Context           mContext;
        core::ShaderManager     mShaderManager;

        /// @brief Increase this in an early init method to get auxiliary command buffers
        uint32_t                                        mAuxiliaryCommandBufferCount = 0;
        std::array<InFlightFrame, INFLIGHT_FRAME_COUNT> mInFlightFrames;
        uint32_t                                        mInFlightFrameIndex = 0;
        uint64_t                                        mRenderedFrameCount = 0;

        fp64_t mLastShadersCheckedTimestamp = 0.0;

        inline static const char* const FRAMERECORDBENCH_WAITONFENCE      = "Wait On Fence";
        inline static const char* const FRAMERECORDBENCH_ACQUIRESWAPIMAGE = "Acquire Swapimage";
        inline static const char* const FRAMERECORDBENCH_RECORDCMDBUFFERS = "Record CmdBuffers";
        inline static const char* const FRAMERECORDBENCH_PRESENT          = "Present";

        bool                 mEnableFrameRecordBenchmark = false;
        bench::HostBenchmark mHostFrameRecordBenchmark;

        event::PriorityReceiver<VkExtent2D> mOnSwapchainResized;
        event::Receiver<const osi::Event*>  mOnOsEvent;
    };
}  // namespace foray::base