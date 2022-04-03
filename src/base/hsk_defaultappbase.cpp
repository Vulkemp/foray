#include "hsk_defaultappbase.hpp"
#include "hsk_logger.hpp"

namespace hsk
{
	void DefaultAppBase::BaseInit()
	{
		// create a window and add its requried instance extensions to the instance builder
		mWindow.Create();
		auto vulkanSurfaceExtensions = mWindow.GetVkSurfaceExtensions();
		for (const char *extension : vulkanSurfaceExtensions)
		{
			mVkbInstanceBuilder.enable_extension(extension);
		}

		mVkbInstanceBuilder.require_api_version(VK_API_VERSION_1_1);

		// create instance using instance builder from minimal app base
		MinimalAppBase::BaseInit();

		// get vulkan surface handle with created instance
		mSurface = mWindow.GetSurfaceKHR(mInstance);

		// create physical device selector
		vkb::PhysicalDeviceSelector pds(mVkbInstance, mSurface);

		{ // Configure device selector

			// Require capability to present to the current windows surface
			pds.require_present();
			pds.set_surface(mSurface);

			// Basic minimums for raytracing
			pds.set_minimum_version(1u, 1u);
			std::vector<const char *> requiredExtensions{
				VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, // acceleration structure
				VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,	  // rt pipeline
				// dependencies of acceleration structure
				VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
				VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
				VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
				// dependencies of rt pipeline
				VK_KHR_SPIRV_1_4_EXTENSION_NAME,
				VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME};
			pds.add_required_extensions(requiredExtensions);
		}

		// allow user to alter phyiscal device selection
		BeforePhysicalDeviceSelection(pds);

		// create phyiscal device
		auto physicalDeviceSelectionReturn = pds.select();
		if (!physicalDeviceSelectionReturn)
		{
			logger()->error("Physical device creation: {}", physicalDeviceSelectionReturn.error().message());
			throw std::exception();
		}
		mVkbPhysicalDevice = physicalDeviceSelectionReturn.value();
		mPhysicalDevice = mVkbPhysicalDevice.physical_device;

		// create logical device builder
		vkb::DeviceBuilder deviceBuilder{mVkbPhysicalDevice};

		{ // Configure logical device builder

			// Adapted from https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/raytracing_extended/raytracing_extended.cpp#L136
			// distributed via Apache 2.0 license https://github.com/KhronosGroup/Vulkan-Samples/blob/master/LICENSE

			// This currently causes a segfault, so commented out for the time being

			// mDeviceFeatures.bdafeatures.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
			// mDeviceFeatures.bdafeatures.bufferDeviceAddress = VK_TRUE;
			// deviceBuilder.add_pNext(&mDeviceFeatures.bdafeatures);

			// mDeviceFeatures.rtpfeatures.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
			// mDeviceFeatures.rtpfeatures.rayTracingPipeline = VK_TRUE;
			// deviceBuilder.add_pNext(&mDeviceFeatures.rtpfeatures);

			// mDeviceFeatures.asfeatures.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
			// mDeviceFeatures.asfeatures.accelerationStructure = VK_TRUE;
			// deviceBuilder.add_pNext(&mDeviceFeatures.asfeatures);

			// mDeviceFeatures.difeatures.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
			// mDeviceFeatures.difeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
			// deviceBuilder.add_pNext(&mDeviceFeatures.difeatures);

			

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
		if (!deviceBuilderReturn)
		{
			logger()->error("Device creation: {}", deviceBuilderReturn.error().message());
			throw std::exception();
		}
		mVkbDevice = deviceBuilderReturn.value();
		mDevice = mVkbDevice.device;

		vkb::SwapchainBuilder swapchainBuilder(mVkbDevice, mSurface);

		BeforeSwapchainBuilding(swapchainBuilder);

		auto swapchainBuilderReturn = swapchainBuilder.build();
		if (!swapchainBuilderReturn)
		{
			logger()->error("Swapchain building: {}", swapchainBuilderReturn.error().message());
			throw std::exception();
		}

		mVkbSwapchain = swapchainBuilderReturn.value();
		mSwapchain = mVkbSwapchain.swapchain;
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
}