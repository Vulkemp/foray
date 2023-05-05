#include "foray_defaultappbase.hpp"
#include "../core/foray_managedresource.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../foray_exception.hpp"
#include "../foray_logger.hpp"
#include "../osi/foray_osi_event.hpp"
#include "../stages/foray_renderstage.hpp"
#include "foray_applicationloop.hpp"

namespace foray::base {
    DefaultAppBase::DefaultAppBase(AppLoopBase* apploop)
        : mAppLoop(apploop)
        , mOsManager()
        , mInstance(
              &mContext,
              [this](vkb::InstanceBuilder& builder) { this->ApiBeforeInstanceCreate(builder); },
#if FORAY_DEBUG || FORAY_VALIDATION  // Set validation layers and debug callbacks on / off
              true
#else
              false
#endif
              )
        , mDevice(
              &mContext,
              [this](vkb::PhysicalDeviceSelector& selector) { this->ApiBeforeDeviceSelection(selector); },
              [this](vkb::DeviceBuilder& builder) { this->ApiBeforeDeviceBuilding(builder); })
        , mWindowSwapchain(
              &mContext,
              [this](osi::Window& window) { this->ApiBeforeWindowCreate(window); },
              [this](vkb::SwapchainBuilder& builder) { this->ApiBeforeSwapchainBuilding(builder); },
              nullptr,
              nullptr)
        , mShaderManager(&mContext)
    {
#if FORAY_DISABLE_RT
        mDevice->SetEnableRaytracingFeaturesAndExtensions(false);
#endif
    }

    void DefaultAppBase::IApplicationInit()
    {
        ApiBeforeInit();

#if FORAY_DEBUG
        logger()->info("Debugging and validation layers enabled");
#endif

        mOsManager.Init();
        mOnOsEvent.Set(mOsManager.OnEvent(), [this](const osi::Event* event) { this->OnOsEvent(event); });
        mContext.OsManager = &mOsManager;

        mWindowSwapchain->CreateWindow();
        mOnSwapchainResized.Set(
            mWindowSwapchain->OnResized(), [this](VkExtent2D extent) { this->ApiOnSwapchainResized(extent); }, -10000000);

        mInstance->Create();
        mDevice->Create();
        mWindowSwapchain->CreateSwapchain();

        InitGetQueue();
        InitCommandPool();
        InitCreateVma();
        InitSyncObjects();

        mSamplerCollection.New(&mContext);
        mContext.SamplerCol = mSamplerCollection.Get();

        mContext.ShaderMan = mShaderManager.Get();

        ApiInit();
    }

    void DefaultAppBase::InitGetQueue()
    {
        // Make sure the graphics queue family supports present and transfer
        auto retPresentQueueIndex = mDevice->GetDevice().get_queue_index(vkb::QueueType::present);
        Assert((bool)retPresentQueueIndex, "Failed to find a queue family supporting present for the configured surface");
        auto vkQueueProperties = mDevice->GetPhysicalDevice().get_queue_families()[*retPresentQueueIndex];

        Assert((vkQueueProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT) > 0, "Present queue does not support transfer");
        Assert((vkQueueProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT) > 0, "Present queue does not support graphics");

        // Get the graphics queue with a helper function
        auto retQueue = mDevice->GetDevice().get_queue(vkb::QueueType::present);
        FORAY_ASSERTFMT(retQueue, "Failed to get queue. Error: {} ", retQueue.error().message())
        mContext.Queue            = *retQueue;
        mContext.QueueFamilyIndex = *retPresentQueueIndex;
    }

    void DefaultAppBase::InitCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
        // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = mContext.QueueFamilyIndex;

        AssertVkResult(mDevice->GetDispatchTable().createCommandPool(&poolInfo, nullptr, &mContext.CommandPool));
    }

    void DefaultAppBase::InitCreateVma()
    {
        VmaVulkanFunctions vulkanFunctions    = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_2;
        allocatorCreateInfo.physicalDevice         = mDevice->GetPhysicalDevice();
        allocatorCreateInfo.device                 = mDevice->GetDevice();
        allocatorCreateInfo.instance               = mInstance->GetInstance();
        allocatorCreateInfo.pVulkanFunctions       = &vulkanFunctions;

        if(mDevice->GetEnableDefaultFeaturesAndExtensions())
        {
            allocatorCreateInfo.flags |= VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

        vmaCreateAllocator(&allocatorCreateInfo, &mContext.Allocator);
    }

    void DefaultAppBase::InitSyncObjects()
    {
        for(auto& frame : mInFlightFrames)
        {
            frame.New(&mContext, mAuxiliaryCommandBufferCount);
        }
    }

    DefaultAppBase::~DefaultAppBase()
    {
        AssertVkResult(mDevice->GetDispatchTable().deviceWaitIdle());

        mOnSwapchainResized.Destroy();
        mOnOsEvent.Destroy();

        mSamplerCollection = nullptr;

        for(Local<InFlightFrame>& frame : mInFlightFrames)
        {
            frame.Delete();
        }

        mDevice->GetDispatchTable().destroyCommandPool(mContext.CommandPool, nullptr);

        core::ManagedResource::sPrintAllocatedResources(true);

        vmaDestroyAllocator(mContext.Allocator);
        mContext.Allocator = nullptr;
        mWindowSwapchain   = nullptr;
        mDevice            = nullptr;
        mInstance          = nullptr;
    }

    void DefaultAppBase::RecreateSwapchain()
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities{};
        AssertVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mDevice->GetPhysicalDevice(), mWindowSwapchain->GetWindow().GetOrCreateSurfaceKHR(mInstance->GetInstance()),
                                                                 &surfaceCapabilities));

        if(surfaceCapabilities.maxImageExtent.width == 0 || surfaceCapabilities.maxImageExtent.height == 0)
        {
            // we cannot rebuild the swapchain if the maximum supported extent has a zero for either height or width
            // for example if the window was minimized
            return;
        }

        mWindowSwapchain->RecreateSwapchain();
    }

    bool DefaultAppBase::IApplicationLoopReady()
    {
        InFlightFrame& currentFrame = mInFlightFrames[mInFlightFrameIndex].GetRef();

        return currentFrame.HasFinishedExecution();
    }

    void DefaultAppBase::IApplicationProcessEvents()
    {
        while(mOsManager.PollEvent())
            ;
    }

    void DefaultAppBase::OnOsEvent(const osi::Event* event)
    {
        ApiOnOsEvent(event);
        if(event->Type == osi::Event::EType::WindowCloseRequested && event->Source && osi::Window::Windows().size() <= 1)
        {
            // The last window has been requested to close, oblige by stopping the renderloop
            mAppLoop->RequestStop();
        }
    }

    void DefaultAppBase::IApplicationLoop(LoopInfo& renderInfo)
    {
        // Check for shader recompilation
        if(mLastShadersCheckedTimestamp + 1 < renderInfo.SinceStart)
        {
            mShaderManager->CheckAndUpdateShaders([this]() { AssertVkResult(this->mContext.DispatchTable().deviceWaitIdle()); });
        }

        if(mEnableFrameRecordBenchmark)
        {
            mHostFrameRecordBenchmark.Begin();
        }

        // Fetch next in flight frame
        InFlightFrame& currentFrame = *mInFlightFrames[mInFlightFrameIndex].Get();

        // Wait for it to finish vkWaitForFences(...)
        currentFrame.WaitForExecutionFinished();

        // The previous time this frame was used would now have query results available
        if(mRenderedFrameCount > INFLIGHT_FRAME_COUNT)
        {
            ApiFrameFinishedExecuting(mRenderedFrameCount - INFLIGHT_FRAME_COUNT);
        }

        if(mEnableFrameRecordBenchmark)
        {
            mHostFrameRecordBenchmark.LogTimestamp(FRAMERECORDBENCH_WAITONFENCE);
        }

        // Acquire the swapchain image
        ESwapchainInteractResult result = currentFrame.AcquireSwapchainImage();

        if(mEnableFrameRecordBenchmark)
        {
            mHostFrameRecordBenchmark.LogTimestamp(FRAMERECORDBENCH_ACQUIRESWAPIMAGE);
        }

        if(result == ESwapchainInteractResult::Resized)
        {  // Recreate swapchain
            RecreateSwapchain();
            if(mEnableFrameRecordBenchmark)
            {
                mHostFrameRecordBenchmark.LogTimestamp(FRAMERECORDBENCH_RECORDCMDBUFFERS);
                mHostFrameRecordBenchmark.LogTimestamp(FRAMERECORDBENCH_PRESENT);
                mHostFrameRecordBenchmark.End();
            }
            return;
        }

        // Reset the fence so it can be signalled again
        currentFrame.ResetFence();


        FrameRenderInfo frameRenderInfo(renderInfo, &currentFrame);
        frameRenderInfo.SetFrameNumber(mRenderedFrameCount).SetRenderSize(mWindowSwapchain->GetSwapchain().extent);

        // Record command buffer
        // The user is expected in here to
        //      - Begin and Submit all their command buffers
        //      - Prepare swapchain image for use, write to it, and prepare it for submit
        ApiRender(frameRenderInfo);

        if(mEnableFrameRecordBenchmark)
        {
            mHostFrameRecordBenchmark.LogTimestamp(FRAMERECORDBENCH_RECORDCMDBUFFERS);
        }

        // Present the swapchain image
        result = currentFrame.Present();


        if(result == ESwapchainInteractResult::Resized)
        {
            // Redo Swapchain
            if(mAppLoop->IsRunning())
            {
                RecreateSwapchain();
            }
        }

        if(mEnableFrameRecordBenchmark)
        {
            mHostFrameRecordBenchmark.LogTimestamp(FRAMERECORDBENCH_PRESENT);
            mHostFrameRecordBenchmark.End();
        }

        // Advance frame index
        mRenderedFrameCount++;
        mInFlightFrameIndex = (mInFlightFrameIndex + 1) % INFLIGHT_FRAME_COUNT;
    }

}  // namespace foray::base