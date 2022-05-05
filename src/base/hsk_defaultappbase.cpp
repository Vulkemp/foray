#include "hsk_defaultappbase.hpp"
#include "../hsk_env.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../hsk_vmaHelpers.hpp"
#include "hsk_logger.hpp"
#include "vma/vk_mem_alloc.h"

namespace hsk {
    void DefaultAppBase::BaseInit()
    {
        logger()->info("Current working directory: {}", CurrentWorkingDirectory());
        // recompile shaders
        BaseInitCompileShaders();

        // create a window and add its requried instance extensions to the instance builder
        mWindow.Create();
        auto vulkanSurfaceExtensions = mWindow.GetVkSurfaceExtensions();
        for(const char* extension : vulkanSurfaceExtensions)
        {
            mVkbInstanceBuilder.enable_extension(extension);
        }

        mVkbInstanceBuilder.require_api_version(VK_API_VERSION_1_3);

        // create instance using instance builder from minimal app base
        MinimalAppBase::BaseInit();

        // get vulkan surface handle with created instance
        mSurface = mWindow.GetSurfaceKHR(mInstance);

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
        vkb::PhysicalDeviceSelector pds(mInstanceVkb, mSurface);

        {  // Configure device selector

            // Require capability to present to the current windows surface
            pds.require_present();
            pds.set_surface(mSurface);

            // Basic minimums for raytracing
            pds.set_minimum_version(1u, 1u);
            std::vector<const char*> requiredExtensions{VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,  // acceleration structure
                                                        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,    // rt pipeline
                                                        // dependencies of acceleration structure
                                                        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                                                        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
                                                        // dependencies of rt pipeline
                                                        VK_KHR_SPIRV_1_4_EXTENSION_NAME, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME};
            pds.add_required_extensions(requiredExtensions);
        }

        // allow user to alter phyiscal device selection
        BeforePhysicalDeviceSelection(pds);

        // create phyiscal device
        auto physicalDeviceSelectionReturn = pds.select();
        if(!physicalDeviceSelectionReturn)
        {
            throw Exception("Physical device creation: {}", physicalDeviceSelectionReturn.error().message().c_str());
        }
        mPhysicalDeviceVkb = physicalDeviceSelectionReturn.value();
        mPhysicalDevice    = mPhysicalDeviceVkb.physical_device;
    }

    void DefaultAppBase::BaseInitBuildDevice()
    {
        // create logical device builder
        vkb::DeviceBuilder deviceBuilder{mPhysicalDeviceVkb};

        {  // Configure logical device builder

            // Adapted from https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/raytracing_extended/raytracing_extended.cpp#L136
            // distributed via Apache 2.0 license https://github.com/KhronosGroup/Vulkan-Samples/blob/master/LICENSE

            // This currently causes a segfault, so commented out for the time being

            mDeviceFeatures.bdafeatures.sType               = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
            mDeviceFeatures.bdafeatures.bufferDeviceAddress = VK_TRUE;
            deviceBuilder.add_pNext(&mDeviceFeatures.bdafeatures);

            mDeviceFeatures.rtpfeatures.sType              = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            mDeviceFeatures.rtpfeatures.rayTracingPipeline = VK_TRUE;
            deviceBuilder.add_pNext(&mDeviceFeatures.rtpfeatures);

            mDeviceFeatures.asfeatures.sType                 = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            mDeviceFeatures.asfeatures.accelerationStructure = VK_TRUE;
            deviceBuilder.add_pNext(&mDeviceFeatures.asfeatures);

            mDeviceFeatures.difeatures.sType                                     = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            mDeviceFeatures.difeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
            deviceBuilder.add_pNext(&mDeviceFeatures.difeatures);


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
        if(!deviceBuilderReturn)
        {
            throw Exception("Device creation: {}", deviceBuilderReturn.error().message());
        }
        mDeviceVkb = deviceBuilderReturn.value();
        mDevice    = mDeviceVkb.device;
    }

    void DefaultAppBase::BaseInitBuildSwapchain()
    {
        vkb::SwapchainBuilder swapchainBuilder(mDeviceVkb, mSurface);

        // default swapchain image formats:
        // color format: VK_FORMAT_B8G8R8A8_SRGB
        // color space: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        swapchainBuilder.use_default_format_selection();

        // tell vulkan the swapchain images will be used as color attachments
        swapchainBuilder.use_default_image_usage_flags();

        // use mailbox if possible, else fallback to fifo
        swapchainBuilder.use_default_present_mode_selection();

        swapchainBuilder.use_default_format_feature_flags();


        BeforeSwapchainBuilding(swapchainBuilder);

        auto swapchainBuilderReturn = swapchainBuilder.build();

        if(!swapchainBuilderReturn)
        {
            throw Exception("Swapchain building: {}", swapchainBuilderReturn.error().message());
        }

        mSwapchainVkb = swapchainBuilderReturn.value();
        mSwapchain    = mSwapchainVkb.swapchain;
    }

    void DefaultAppBase::BaseInitGetVkQueues()
    {
        // Get the graphics queue with a helper function
        auto defaultQueueReturn = mDeviceVkb.get_queue(vkb::QueueType::graphics);
        if(!defaultQueueReturn)
        {
            throw Exception("Failed to get graphics queue. Error: {} ", defaultQueueReturn.error().message());
        }
        mDefaultQueue.Queue            = defaultQueueReturn.value();
        mDefaultQueue.QueueFamilyIndex = mDeviceVkb.get_queue_index(vkb::QueueType::graphics).value();

        auto presentQueueReturn = mDeviceVkb.get_queue(vkb::QueueType::present);
        if(!presentQueueReturn)
        {
            throw Exception("Failed to get graphics queue. Error: {} ", presentQueueReturn.error().message());
        }
        mPresentQueue.Queue            = presentQueueReturn.value();
        mPresentQueue.QueueFamilyIndex = mDeviceVkb.get_queue_index(vkb::QueueType::present).value();
    }

    void DefaultAppBase::BaseInitCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
        // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = mDefaultQueue.QueueFamilyIndex;

        VkResult result = vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPoolDefault);
        if(result != VK_SUCCESS)
        {
            throw Exception("failed to create command pool! VkResult: {}", result);
        }
    }

    void DefaultAppBase::BaseInitCreateVma()
    {
        VmaVulkanFunctions vulkanFunctions    = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_2;
        allocatorCreateInfo.physicalDevice         = mPhysicalDevice;
        allocatorCreateInfo.device                 = mDevice;
        allocatorCreateInfo.instance               = mInstance;
        allocatorCreateInfo.pVulkanFunctions       = &vulkanFunctions;

        vmaCreateAllocator(&allocatorCreateInfo, &mAllocator);
    }

    void DefaultAppBase::BaseInitCompileShaders()
    {
        logger()->info("Compiling shaders...");
        std::string shaderSourceDirectory = CurrentWorkingDirectory() + mShaderSubdir;
        std::string shaderOutputDirectory = shaderSourceDirectory;
        if(mShaderSourceDirectoryPathFull.length() > 0)
        {

            shaderSourceDirectory = mShaderSourceDirectoryPathFull;
        }

        if(mShaderOutputDirectoryPathFull.length() > 0)
        {

            shaderOutputDirectory = mShaderOutputDirectoryPathFull;
        }

        mShaderCompiler.Init(shaderSourceDirectory, shaderOutputDirectory);
        mShaderCompiler.CompileAll();
        logger()->info("Compiling shaders successfully finished!");
    }

    void DefaultAppBase::BaseInitSyncObjects()
    {
        auto images     = mSwapchainVkb.get_images().value();
        auto imageviews = mSwapchainVkb.get_image_views().value();
        for(uint32_t i = 0; i < mSwapchainVkb.image_count; i++)
        {
            PresentTarget target{};
            target.Image         = images[i];
            target.ImageView     = imageviews[i];
            target.CommandBuffer = createCommandBuffer(mDevice, mCommandPoolDefault, VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);

            VkSemaphoreCreateInfo semaphoreCI{};
            semaphoreCI.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            AssertVkResult(vkCreateSemaphore(mDevice, &semaphoreCI, nullptr, &target.Ready));
            AssertVkResult(vkCreateSemaphore(mDevice, &semaphoreCI, nullptr, &target.Finished));

            mPresentTargets.push_back(std::move(target));
        }
    }

    void DefaultAppBase::BaseCleanupVulkan()
    {
        vkDestroyCommandPool(mDevice, mCommandPoolDefault, nullptr);
        for(auto& target : mPresentTargets)
        {
            vkDestroySemaphore(mDevice, target.Ready, nullptr);
            vkDestroySemaphore(mDevice, target.Finished, nullptr);
            vkDestroyImageView(mDevice, target.ImageView, nullptr);
        }

        vkb::destroy_swapchain(mSwapchainVkb);
        mSwapchain = nullptr;
        vkb::destroy_device(mDeviceVkb);
        mDevice = nullptr;
        vkb::destroy_surface(mInstanceVkb, mSurface);
        mSurface = nullptr;
        mWindow.Destroy();
        MinimalAppBase::BaseCleanupVulkan();
    }

    void DefaultAppBase::Render(float delta)
    {
        // BasePrepareFrame();
        // RecordCommandBuffer(CurrentTarget().CommandBuffer);
        // BaseSubmitFrame();
    }
    void DefaultAppBase::BasePrepareFrame()
    {
        mPreviousPresentIndex = mCurrentPresentIndex;
        vkAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX, CurrentTarget().Ready, VK_NULL_HANDLE, &mCurrentPresentIndex);
        vkResetCommandBuffer(CurrentTarget().CommandBuffer, 0);
    }
    void DefaultAppBase::BaseSubmitFrame()
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // VkSemaphore          waitSemaphores[] = {imageAvailableSemaphore};
        // VkPipelineStageFlags waitStages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        // submitInfo.waitSemaphoreCount         = 1;
        // submitInfo.pWaitSemaphores            = waitSemaphores;
        // submitInfo.pWaitDstStageMask          = waitStages;
    }
}  // namespace hsk