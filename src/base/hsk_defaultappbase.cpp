#include "hsk_defaultappbase.hpp"
#include "hsk_logger.hpp"

namespace hsk {
    void DefaultAppBase::BaseInit()
    {
        // create a window and add its requried instance extensions to the instance builder
        mWindow.Create();
        auto vulkanSurfaceExtensions = mWindow.GetVkSurfaceExtensions();
        for(const char* extension : vulkanSurfaceExtensions)
        {
            mVkbInstanceBuilder.enable_extension(extension);
        }

        mVkbInstanceBuilder.require_api_version(VK_API_VERSION_1_1);

        // create instance using instance builder from minimal app base
        MinimalAppBase::BaseInit();

        // get vulkan surface handle with created instance
        mSurface = mWindow.GetSurfaceKHR(mInstance);

        BaseInitSelectPhysicalDevice();
        BaseInitBuildDevice();
        BaseInitBuildSwapchain();
        
    }

    void DefaultAppBase::BaseInitSelectPhysicalDevice() {
        // create physical device selector
        vkb::PhysicalDeviceSelector pds(mVkbInstance, mSurface);

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
            logger()->error("Physical device creation: {}", physicalDeviceSelectionReturn.error().message());
            throw std::exception();
        }
        mVkbPhysicalDevice = physicalDeviceSelectionReturn.value();
        mPhysicalDevice    = mVkbPhysicalDevice.physical_device;
    }

    void DefaultAppBase::BaseInitBuildDevice() {
        // create logical device builder
        vkb::DeviceBuilder deviceBuilder{mVkbPhysicalDevice};

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
            logger()->error("Device creation: {}", deviceBuilderReturn.error().message());
            throw std::exception();
        }
        mVkbDevice = deviceBuilderReturn.value();
        mDevice    = mVkbDevice.device;
    }

    void DefaultAppBase::BaseInitBuildSwapchain() {
        vkb::SwapchainBuilder swapchainBuilder(mVkbDevice, mSurface);
        
        // Swapchain presentation modes, description from vulkan-tutorial.com
        // VK_PRESENT_MODE_IMMEDIATE_KHR:
        //      Images submitted by your application are transferred to the screen right away,
        //      which may result in tearing.
        // 
        // VK_PRESENT_MODE_FIFO_KHR: 
        //      The swap chain is a queue where the display takes an image from the front of the queue
        //      when the display is refreshed and the program inserts rendered images at the back of the queue.
        //      If the queue is full then the program has to wait. This is most similar to vertical sync as found in modern games.
        //      The moment that the display is refreshed is known as "vertical blank".
        // 
        // VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        //      This mode only differs from the previous one if the application is late and the queue was empty at the last vertical blank.
        //      Instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives. This may result in visible tearing.
        // 
        // VK_PRESENT_MODE_MAILBOX_KHR:
        //      This is another variation of the second mode. Instead of blocking the application when the queue is full,
        //      the images that are already queued are simply replaced with the newer ones.
        //      This mode can be used to render frames as fast as possible while still avoiding tearing,
        //      resulting in fewer latency issues than standard vertical sync. This is commonly known as "triple buffering",
        //      although the existence of three buffers alone does not necessarily mean that the framerate is unlocked.
        
        // use mailbox if possible, else fallback to fifo
        swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR);
        swapchainBuilder.add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR);

        // TODO: simple query for the desired format
        //swapchainBuilder.set_desired_format(VK_FORMAT_B8G8R8A8_SRGB);

        BeforeSwapchainBuilding(swapchainBuilder);

        auto swapchainBuilderReturn = swapchainBuilder.build();

        if(!swapchainBuilderReturn)
        {
            logger()->error("Swapchain building: {}", swapchainBuilderReturn.error().message());
            throw std::exception();
        }

        mVkbSwapchain = swapchainBuilderReturn.value();
        mSwapchain    = mVkbSwapchain.swapchain;
    }

    void DefaultAppBase::BaseCleanupVulkan()
    {
        vkb::destroy_swapchain(mVkbSwapchain);
        mSwapchain = nullptr;
        vkb::destroy_device(mVkbDevice);
        mDevice = nullptr;
        vkb::destroy_surface(mVkbInstance, mSurface);
        mSurface = nullptr;
        mWindow.Destroy();
        MinimalAppBase::BaseCleanupVulkan();
    }
}  // namespace hsk