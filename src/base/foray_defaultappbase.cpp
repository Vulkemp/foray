#include "foray_defaultappbase.hpp"
#include "../core/foray_deviceresource.hpp"
#include "../core/foray_managedimage.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../foray_exception.hpp"
#include "../foray_logger.hpp"
#include "../foray_vma.hpp"
#include "../foray_vulkan.hpp"
#include "../osi/foray_env.hpp"

namespace foray::base {
    void DefaultAppBase::BaseInit()
    {
        mContext.DebugEnabled = mDebugEnabled;

        logger()->info("Debugging and validation layers enabled : {}", mDebugEnabled);

        // recompile shaders
        // BaseInitCompileShaders();
        // ShaderManager::
        core::ShaderManager::Instance().CheckAndUpdateShaders();

        // create a window and add its requried instance extensions to the instance builder
        mContext.ContextSwapchain.Window.Create();
        auto vulkanSurfaceExtensions = mContext.ContextSwapchain.Window.GetVkSurfaceExtensions();
        for(const char* extension : vulkanSurfaceExtensions)
        {
            mVkbInstanceBuilder.enable_extension(extension);
        }
        SetWindowDisplayMode(foray::EDisplayMode::WindowedResizable);

        mVkbInstanceBuilder.require_api_version(mRequiredVulkanApiVersion);

        // create instance using instance builder from minimal app base
        MinimalAppBase::BaseInit();

        // get vulkan surface handle with created instance
        mContext.ContextSwapchain.Surface = mContext.ContextSwapchain.Window.GetSurfaceKHR(mInstance);
        mContext.Instance                 = mInstanceVkb;

        BaseInitSelectPhysicalDevice();
        BaseInitBuildDevice();
        BaseInitBuildSwapchain();
        BaseInitGetVkQueues();
        BaseInitCommandPool();
        BaseInitCreateVma();
        BaseInitSyncObjects();
    }

    void DefaultAppBase::BaseInitSelectPhysicalDevice()
    {
        // create physical device selector
        vkb::PhysicalDeviceSelector pds(mInstanceVkb, mContext.ContextSwapchain.Surface);

        {  // Configure device selector

            // Require capability to present to the current windows surface
            pds.require_present();
            pds.set_surface(mContext.ContextSwapchain.Surface);

            //// Basic minimums for raytracing
#ifndef DISABLE_RT_EXTENSIONS
            pds.set_minimum_version(1u, 1u);
            std::vector<const char*> requiredExtensions{
                VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,  // acceleration structure
                VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,    // rt pipeline
                // dependencies of acceleration structure
                VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
                // dependencies of rt pipeline
                VK_KHR_SPIRV_1_4_EXTENSION_NAME,
                VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
                // Relaxed block layout allows custom strides for buffer layouts. Used for index buffer and vertex buffer in rt shaders
                VK_KHR_RELAXED_BLOCK_LAYOUT_EXTENSION_NAME,
            };
            pds.add_required_extensions(requiredExtensions);
#endif
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        pds.set_required_features(deviceFeatures);

        // allow user to alter phyiscal device selection
        BeforePhysicalDeviceSelection(pds);

        // create phyiscal device
        auto physicalDeviceSelectionReturn = pds.select();
        FORAY_ASSERTFMT(physicalDeviceSelectionReturn, "Physical device creation: {}", physicalDeviceSelectionReturn.error().message().c_str())

        mContext.PhysicalDevice = physicalDeviceSelectionReturn.value();
    }

    void DefaultAppBase::BaseInitBuildDevice()
    {
        // create logical device builder
        vkb::DeviceBuilder deviceBuilder{mContext.PhysicalDevice};

        {  // Configure logical device builder

            // Adapted from https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/raytracing_extended/raytracing_extended.cpp#L136
            // distributed via Apache 2.0 license https://github.com/KhronosGroup/Vulkan-Samples/blob/master/LICENSE

#ifndef DISABLE_RT_EXTENSIONS
            mDeviceFeatures.BufferDeviceAdressFeatures.sType               = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
            mDeviceFeatures.BufferDeviceAdressFeatures.bufferDeviceAddress = VK_TRUE;
            deviceBuilder.add_pNext(&mDeviceFeatures.BufferDeviceAdressFeatures);

            mDeviceFeatures.RayTracingPipelineFeatures.sType              = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            mDeviceFeatures.RayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
            deviceBuilder.add_pNext(&mDeviceFeatures.RayTracingPipelineFeatures);

            mDeviceFeatures.AccelerationStructureFeatures.sType                 = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            mDeviceFeatures.AccelerationStructureFeatures.accelerationStructure = VK_TRUE;
            deviceBuilder.add_pNext(&mDeviceFeatures.AccelerationStructureFeatures);
#endif

            mDeviceFeatures.DescriptorIndexingFeatures.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            mDeviceFeatures.DescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
            mDeviceFeatures.DescriptorIndexingFeatures.runtimeDescriptorArray                    = VK_TRUE;  // enable this for unbound descriptor arrays
            deviceBuilder.add_pNext(&mDeviceFeatures.DescriptorIndexingFeatures);

            mDeviceFeatures.Sync2FEatures =
                VkPhysicalDeviceSynchronization2Features{.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES, .synchronization2 = VK_TRUE};
            deviceBuilder.add_pNext(&mDeviceFeatures.Sync2FEatures);

            // This currently causes a segfault, so commented out for the time being

            // auto &features = mVkbPhysicalDevice.request_extension_features<VkPhysicalDeviceDescriptorIndexingFeaturesEXT>(
            // 	VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT);
            // features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

            // if (mVkbPhysicalDevice.get_features().samplerAnisotropy)
            // {
            // 	mVkbPhysicalDevice.get_mutable_requested_features().samplerAnisotropy = true;
            // }
        }


        // allow user to alter device building
        BeforeDeviceBuilding(deviceBuilder);

        // automatically propagate needed data from instance & physical device
        auto deviceBuilderReturn = deviceBuilder.build();
        FORAY_ASSERTFMT(deviceBuilderReturn, "Device creation: {}", deviceBuilderReturn.error().message())
        mContext.Device        = deviceBuilderReturn.value();
        mContext.DispatchTable = mContext.Device.make_table();
    }

    void DefaultAppBase::BaseInitBuildSwapchain()
    {
        vkb::SwapchainBuilder swapchainBuilder(mContext.Device, mContext.ContextSwapchain.Surface);

        // default swapchain image formats:
        // color format: VK_FORMAT_B8G8R8A8_SRGB
        // color space: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        swapchainBuilder.use_default_format_selection();

        // tell vulkan the swapchain images will be used as color attachments
        swapchainBuilder.use_default_image_usage_flags();
        swapchainBuilder.add_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        // use mailbox if possible, else fallback to fifo
        swapchainBuilder.use_default_present_mode_selection();
        swapchainBuilder.use_default_format_feature_flags();
        swapchainBuilder.add_format_feature_flags(VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_BLIT_DST_BIT | VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_TRANSFER_DST_BIT
                                                  | VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

        // allow user app to modify swapchain building
        BeforeSwapchainBuilding(swapchainBuilder);

        auto swapchainBuilderReturn = swapchainBuilder.build();
        FORAY_ASSERTFMT(swapchainBuilderReturn, "Swapchain building: {}", swapchainBuilderReturn.error().message())

        mContext.Swapchain = swapchainBuilderReturn.value();

        // extract swapchain images
        auto images     = mContext.Swapchain.get_images();
        auto imageviews = mContext.Swapchain.get_image_views();
        Assert(images.has_value(), "Failed to acquire swapchain images!");
        Assert(imageviews.has_value(), "Failed to acquire swapchain image views!");
        mContext.ContextSwapchain.SwapchainImages.resize(mContext.Swapchain.image_count);
        for(uint32_t i = 0; i < mContext.Swapchain.image_count; i++)
        {
            core::SwapchainImage& swapImage = mContext.ContextSwapchain.SwapchainImages[i];

            swapImage = core::SwapchainImage{
                .Name        = fmt::format("Swapchain image {}", i),
                .Image       = images.value()[i],
                .ImageView   = imageviews.value()[i],
                .ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            SetVulkanObjectName(&mContext, VkObjectType::VK_OBJECT_TYPE_IMAGE, swapImage.Image, swapImage.Name);
        }
    }

    void DefaultAppBase::BaseInitGetVkQueues()
    {
        // Get the graphics queue with a helper function
        auto defaultQueueReturn = mContext.Device.get_queue(vkb::QueueType::graphics);
        FORAY_ASSERTFMT(defaultQueueReturn, "Failed to get graphics queue. Error: {} ", defaultQueueReturn.error().message())

        auto presentQueueReturn = mContext.Device.get_queue(vkb::QueueType::present);
        FORAY_ASSERTFMT(presentQueueReturn, "Failed to get graphics queue. Error: {} ", presentQueueReturn.error().message())

        mContext.QueueGraphics.Queue            = defaultQueueReturn.value();  // TODO: FIX ME: use a dedicated transfer queue in some cases?
        mContext.QueueGraphics.QueueFamilyIndex = mContext.Device.get_queue_index(vkb::QueueType::graphics).value();

        mContext.TransferQueue.Queue            = mContext.QueueGraphics;  // TODO: FIX ME: use a dedicated transfer queue in some cases?
        mContext.TransferQueue.QueueFamilyIndex = mContext.QueueGraphics;

        mContext.PresentQueue.Queue            = presentQueueReturn.value();
        mContext.PresentQueue.QueueFamilyIndex = mContext.Device.get_queue_index(vkb::QueueType::present).value();
    }

    void DefaultAppBase::BaseInitCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
        // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = mContext.QueueGraphics;

        AssertVkResult(vkCreateCommandPool(mContext.Device, &poolInfo, nullptr, &mCommandPoolDefault));
        mContext.CommandPool = mCommandPoolDefault;

        mContext.TransferCommandPool = mCommandPoolDefault;  // TODO: FIX ME: dedicated transfer command pool in some cases?
    }

    void DefaultAppBase::BaseInitCreateVma()
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

#ifndef DISABLE_RT_EXTENSIONS
        // enable calls to GetBufferDeviceAdress
        allocatorCreateInfo.flags |= VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
#endif

        vmaCreateAllocator(&allocatorCreateInfo, &mContext.Allocator);
    }

    void DefaultAppBase::BaseInitSyncObjects()
    {
        mInFlightFrameCount          = INFLIGHT_FRAME_COUNT;
        mAuxiliaryCommandBufferCount = 1;
        BeforeSyncObjectCreation(mInFlightFrameCount, mAuxiliaryCommandBufferCount);

        // https://www.asawicki.info/news_1647_vulkan_bits_and_pieces_synchronization_in_a_frame

        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;

        // auto images     = mSwapchainVkb.get_images().value();
        // auto imageviews = mSwapchainVkb.get_image_views().value();
        mFrames.resize(mInFlightFrameCount);
        for(auto& frame : mFrames)
        {
            frame = std::make_unique<InFlightFrame>();
            frame->Create(&mContext, mAuxiliaryCommandBufferCount);
        }
    }

    void DefaultAppBase::BaseCleanupVulkan()
    {
        AssertVkResult(vkDeviceWaitIdle(mContext.Device));

        mFrames.clear();
        vkDestroyCommandPool(mContext.Device, mCommandPoolDefault, nullptr);

        BaseCleanupSwapchain();

        for(auto deviceResource : *core::DeviceResourceBase::GetTotalAllocatedResources())
        {
            if(deviceResource->Exists())
            {
                logger()->error("Resource with name \"{}\" has not been cleaned up!", deviceResource->GetName());
            }
        }
        vmaDestroyAllocator(mContext.Allocator);

        vkb::destroy_device(mContext.Device);
        mContext.Device = vkb::Device{};
        vkb::destroy_surface(mInstanceVkb, mContext.ContextSwapchain.Surface);
        mContext.ContextSwapchain.Surface = nullptr;
        mContext.ContextSwapchain.Window.Destroy();
        MinimalAppBase::BaseCleanupVulkan();
    }

    void DefaultAppBase::BaseCleanupSwapchain()
    {
        for(auto& image : mContext.ContextSwapchain.SwapchainImages)
        {
            vkDestroyImageView(mContext.Device, image.ImageView, nullptr);
            // vkDestroyImage(mDevice, image.Image, nullptr); Do not destroy images from swapchain
        }
        mContext.ContextSwapchain.SwapchainImages.resize(0);
        vkb::destroy_swapchain(mContext.Swapchain);
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

        AssertVkResult(vkDeviceWaitIdle(mContext.Device));

        BaseCleanupSwapchain();

        if(!mContext.ContextSwapchain.Window.Exists())
        {
            return;
        }

        BaseInitBuildSwapchain();

        OnResized(mContext.Swapchain.extent);
    }

    bool DefaultAppBase::CanRenderNextFrame()
    {
        InFlightFrame* currentFrame = mFrames[mCurrentFrameIndex].get();

        return currentFrame->HasFinishedExecution();
    }

    void DefaultAppBase::Render(float delta)
    {
        InFlightFrame* currentFrame       = mFrames[mCurrentFrameIndex].get();
        uint32_t       previousFrameIndex = (mRenderedFrameCount - 1 + INFLIGHT_FRAME_COUNT) % INFLIGHT_FRAME_COUNT;
        InFlightFrame* previousFrame      = mFrames[previousFrameIndex].get();

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

    void DefaultAppBase::SetWindowDisplayMode(foray::EDisplayMode displayMode)
    {
        mContext.ContextSwapchain.Window.DisplayMode(displayMode);
    }
}  // namespace foray