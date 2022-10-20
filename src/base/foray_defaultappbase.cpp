#include "foray_defaultappbase.hpp"
#include "../core/foray_deviceresource.hpp"
#include "../core/foray_managedimage.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../foray_exception.hpp"
#include "../foray_logger.hpp"
#include "../foray_vma.hpp"
#include "../foray_vulkan.hpp"
#include "../osi/foray_env.hpp"
#include "../stages/foray_renderstage.hpp"

namespace foray::base {
    void DefaultAppBase::Init()
    {
        ApiBeforeInit();

        logger()->info("Debugging and validation layers enabled : {}", mContext.DebugEnabled);

        mOsManager.Init();
        mWindow.CreateWindow();
        mInstance.Create();
        VkSurfaceKHR surface = mWindow.GetWindow().GetSurfaceKHR(mInstance);
        mDevice.Create(mInstance.GetInstance(), surface);
        mWindow.SetDispatchTable(&mDevice.GetDispatchTable());
        mWindow.CreateSwapchain(mInstance, mDevice.GetDevice());

        InitGetVkQueues();
        InitCommandPool();
        InitCreateVma();
        InitSyncObjects();
    }

    void DefaultAppBase::BeforeInstanceCreate(vkb::InstanceBuilder& instanceBuilder)
    {
        auto vulkanSurfaceExtensions = mWindow.GetWindow().GetVkSurfaceExtensions();
        for(const char* extension : vulkanSurfaceExtensions)
        {
            instanceBuilder.enable_extension(extension);
        }
        instanceBuilder.require_api_version(mRequiredVulkanApiVersion);
        ApiBeforeInstanceCreate(instanceBuilder);
    }

    void DefaultAppBase::InitGetVkQueues()
    {
        // Get the graphics queue with a helper function
        auto defaultQueueReturn = mDevice.GetDevice().get_queue(vkb::QueueType::graphics);
        FORAY_ASSERTFMT(defaultQueueReturn, "Failed to get graphics queue. Error: {} ", defaultQueueReturn.error().message())

        auto presentQueueReturn = mContext.GetDevice().get_queue(vkb::QueueType::present);
        FORAY_ASSERTFMT(presentQueueReturn, "Failed to get present queue. Error: {} ", presentQueueReturn.error().message())

        mContext.QueueGraphics.Queue            = defaultQueueReturn.value();  // TODO: FIX ME: use a dedicated transfer queue in some cases?
        mContext.QueueGraphics.QueueFamilyIndex = mContext.Device.get_queue_index(vkb::QueueType::graphics).value();

        mContext.TransferQueue.Queue            = mContext.QueueGraphics;  // TODO: FIX ME: use a dedicated transfer queue in some cases?
        mContext.TransferQueue.QueueFamilyIndex = mContext.QueueGraphics;

        mContext.PresentQueue.Queue            = presentQueueReturn.value();
        mContext.PresentQueue.QueueFamilyIndex = mContext.Device.get_queue_index(vkb::QueueType::present).value();
    }

    void DefaultAppBase::InitCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
        // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = mContext.QueueGraphics;

        AssertVkResult(vkCreateCommandPool(mContext.Device, &poolInfo, nullptr, &mCommandPool));
        mContext.CommandPool = mCommandPool;

        mContext.TransferCommandPool = mCommandPool;  // TODO: FIX ME: dedicated transfer command pool in some cases?
    }

    void DefaultAppBase::InitCreateVma()
    {
        VmaVulkanFunctions vulkanFunctions    = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_2;
        allocatorCreateInfo.physicalDevice         = mContext.PhysicalDevice;
        allocatorCreateInfo.device                 = mContext.Device;
        allocatorCreateInfo.instance               = mInstance;
        allocatorCreateInfo.pVulkanFunctions       = &vulkanFunctions;

        if(mDevice.GetEnableDefaultDeviceFeatures())
        {
            allocatorCreateInfo.flags |= VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

        vmaCreateAllocator(&allocatorCreateInfo, &mContext.Allocator);
    }

    void DefaultAppBase::InitSyncObjects()
    {
        for(auto& frame : mInFlightFrames)
        {
            frame.Create(&mContext, mAuxiliaryCommandBufferCount);
        }
    }

    void DefaultAppBase::Destroy()
    {
        AssertVkResult(vkDeviceWaitIdle(mContext.Device));

        ApiDestroy();

        for(InFlightFrame& frame : mInFlightFrames)
        {
            frame.Destroy();
        }
        vkDestroyCommandPool(mContext.Device, mCommandPool, nullptr);

        for(auto deviceResource : *core::DeviceResourceBase::GetTotalAllocatedResources())
        {
            if(deviceResource->Exists())
            {
                logger()->error("Resource with name \"{}\" has not been cleaned up!", deviceResource->GetName());
            }
        }

        vmaDestroyAllocator(mContext.Allocator);
        mWindow.Destroy();
        mDevice.Destroy();
        mInstance.Destroy();
    }

    void DefaultAppBase::RecreateSwapchain()
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities{};
        AssertVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mContext.PhysicalDevice, mContext.ContextSwapchain.Surface, &surfaceCapabilities));

        if(surfaceCapabilities.maxImageExtent.width == 0 || surfaceCapabilities.maxImageExtent.height == 0)
        {
            // we cannot rebuild the swapchain if the maximum supported extent has a zero for either height or width
            // for example if the window was minimized
            return;
        }

        mWindow.RecreateSwapchain();
    }

    bool DefaultAppBase::CanRenderNextFrame()
    {
        InFlightFrame& currentFrame = mInFlightFrames[mInFlightFrameIndex];

        return currentFrame.HasFinishedExecution();
    }

    void DefaultAppBase::Render(float delta)
    {
        InFlightFrame& currentFrame = mInFlightFrames[mInFlightFrameIndex];

        currentFrame->WaitForExecutionFinished();

        if(mRenderedFrameCount > mFrames.size())
        {
            QueryResultsAvailable(mRenderedFrameCount - mFrames.size());
        }

        VkImage primaryOutput    = nullptr;
        VkImage comparisonOutput = nullptr;

        // Record Render Command buffer
        // clang-format off
        FrameRenderInfo renderInfo;
        renderInfo.SetFrameObjectsIndex(mCurrentFrameIndex)
                  .SetInFlightFrame(currentFrame)
                  .SetFrameNumber(mRenderedFrameCount)
                  .SetFrameTime(delta);
        // cland-format on
        
        // Get the next image index TODO: This action can be deferred until the command buffer section using the swapchain image is required. Should not be necessary however assuming sufficient in flight frames
        uint32_t swapChainImageIndex = 0;
        ESwapchainInteractResult result = currentFrame->AcquireSwapchainImage();

        if(result == ESwapchainInteractResult::Resized)
        {
            RecreateSwapchain();
            return;
        }

        currentFrame->ResetFence();

        // Record command buffer
        RecordCommandBuffer(renderInfo);

        result = currentFrame->Present();

        if(result == ESwapchainInteractResult::Resized)
        {
            // Redo Swapchain
            if (mState == EState::Running)
            {
                RecreateSwapchain();
            }
        }

        // Advance frame index
        mRenderedFrameCount++;
        mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mFrames.size();
    }

    void DefaultAppBase::Update(float delta)
    {
        static float deltaSum = 0;
        deltaSum += delta;
        if (deltaSum > 1.0f)
        {
            deltaSum = 0;
            if (core::ShaderManager::Instance().CheckAndUpdateShaders())
            {
                OnShadersRecompiled();
            }
        }
    }

    void DefaultAppBase::BasePrepareFrame() {}
    void DefaultAppBase::BaseSubmitFrame() {}

    void DefaultAppBase::OnResized(VkExtent2D size)
    {
        for (stages::RenderStage* stage : mRegisteredStages)
        {
            stage->OnResized(size);
        }
    }

    void DefaultAppBase::OnShadersRecompiled()
    {
        for (stages::RenderStage* stage : mRegisteredStages)
        {
            stage->OnShadersRecompiled();
        }
    }

    void DefaultAppBase::RegisterRenderStage(stages::RenderStage* stage) 
    {
        mRegisteredStages.emplace(stage);
    }

    void DefaultAppBase::UnregisterRenderStage(stages::RenderStage* stage) 
    {
        mRegisteredStages.erase(stage);
    }

    void DefaultAppBase::SetWindowDisplayMode(foray::EDisplayMode displayMode)
    {
        mContext.ContextSwapchain.Window.DisplayMode(displayMode);
    }
}  // namespace foray