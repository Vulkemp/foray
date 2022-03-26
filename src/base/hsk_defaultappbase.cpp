#include "hsk_defaultappbase.hpp"
#include "hsk_logger.hpp"

namespace hsk
{
	void DefaultAppBase::BaseInit()
	{
		// create a window and add its requried instance extensions to the instance builder
		mWindow.Create();
		auto vulkanSurfaceExtensions = mWindow.GetVkSurfaceExtensions();
		for (const char* extension : vulkanSurfaceExtensions)
		{
			mVkbInstanceBuilder.enable_extension(extension);
		}

		// create instance using instance builder from minimal app base
		MinimalAppBase::BaseInit();

		// get vulkan surface handle with created instance
		mSurface = mWindow.GetSurfaceKHR(mInstance);

		// create physical device selector
		vkb::PhysicalDeviceSelector pds(mVkbInstance, mSurface);

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
		vkb::DeviceBuilder deviceBuilder{ mVkbPhysicalDevice };

		// allow user to alter device building
		BeforeDeviceBuilding(deviceBuilder);

		// automatically propagate needed data from instance & physical device
		auto deviceBuilderReturn = deviceBuilder.build();
		if (!deviceBuilderReturn) {
			logger()->error("Device creation: {}", deviceBuilderReturn.error().message());
			throw std::exception();
		}
		mVkbDevice = deviceBuilderReturn.value();
		mDevice = mVkbDevice.device;

	}
}