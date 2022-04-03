#pragma once
#include "hsk_minimalappbase.hpp"
#include "../osi/hsk_window.hpp"

namespace hsk
{
	/// @brief Intended as base class for demo applications. Compared to MinimalAppBase it offers a complete simple vulkan setup.
	class DefaultAppBase : public MinimalAppBase
	{
	public:
		DefaultAppBase() = default;
		virtual ~DefaultAppBase() = default;

	protected:
		inline virtual void BeforeInstanceCreate(vkb::InstanceBuilder &instanceBuilder) override;

		/// @brief Alter physical device selection.
		inline virtual void BeforePhysicalDeviceSelection(vkb::PhysicalDeviceSelector &pds);

		/// @brief Alter device selection.
		inline virtual void BeforeDeviceBuilding(vkb::DeviceBuilder &deviceBuilder);

		/// @brief Before building the swapchain
		inline virtual void BeforeSwapchainBuilding(vkb::SwapchainBuilder &swapchainBuilder);

		/// @brief Base init is heavily overriden by this class, because a complete simple vulkan setup is included.
		virtual void BaseInit() override;

		virtual void BaseCleanupVulkan() override;

		/// @brief The main window used for rendering.
		Window mWindow;

#pragma region Vulkan
		VkSurfaceKHR mSurface{};

		vkb::PhysicalDevice mVkbPhysicalDevice;
		VkPhysicalDevice mPhysicalDevice{};

		vkb::Device mVkbDevice;
		VkDevice mDevice{};

		vkb::Swapchain mVkbSwapchain;
		VkSwapchainKHR mSwapchain;

		struct {
			VkPhysicalDeviceBufferDeviceAddressFeatures bdafeatures;
			VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtpfeatures;
			VkPhysicalDeviceAccelerationStructureFeaturesKHR asfeatures;
			VkPhysicalDeviceDescriptorIndexingFeaturesEXT difeatures;
		} mDeviceFeatures;

#pragma endregion
	};

	inline void DefaultAppBase::BeforeInstanceCreate(vkb::InstanceBuilder &instanceBuilder)
	{
		instanceBuilder.require_api_version(VK_API_VERSION_1_1);
	}

	inline void DefaultAppBase::BeforePhysicalDeviceSelection(vkb::PhysicalDeviceSelector &pds)
	{
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

	inline void DefaultAppBase::BeforeDeviceBuilding(vkb::DeviceBuilder &deviceBuilder)
	{
		// Adapted from https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/raytracing_extended/raytracing_extended.cpp#L136
		// distributed via Apache 2.0 license https://github.com/KhronosGroup/Vulkan-Samples/blob/master/LICENSE

		/* This currently causes a segfault, so commented out for the time being

		mDeviceFeatures.bdafeatures.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		mDeviceFeatures.bdafeatures.bufferDeviceAddress = VK_TRUE;
		deviceBuilder.add_pNext(&mDeviceFeatures.bdafeatures);

		mDeviceFeatures.rtpfeatures.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		mDeviceFeatures.rtpfeatures.rayTracingPipeline = VK_TRUE;
		deviceBuilder.add_pNext(&mDeviceFeatures.rtpfeatures);

		mDeviceFeatures.asfeatures.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		mDeviceFeatures.asfeatures.accelerationStructure = VK_TRUE;
		deviceBuilder.add_pNext(&mDeviceFeatures.asfeatures);

		mDeviceFeatures.difeatures.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
		mDeviceFeatures.difeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		deviceBuilder.add_pNext(&mDeviceFeatures.difeatures);

		*/

		// auto &features = mVkbPhysicalDevice.request_extension_features<VkPhysicalDeviceDescriptorIndexingFeaturesEXT>(
		// 	VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT);
		// features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

		// if (mVkbPhysicalDevice.get_features().samplerAnisotropy)
		// {
		// 	mVkbPhysicalDevice.get_mutable_requested_features().samplerAnisotropy = true;
		// }
	}

	inline void DefaultAppBase::BeforeSwapchainBuilding(vkb::SwapchainBuilder &swapchainBuilder)
	{
	}
}