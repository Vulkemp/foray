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
		/// @brief Alter physical device selection.
		virtual void BeforePhysicalDeviceSelection(vkb::PhysicalDeviceSelector& pds) {};

		/// @brief Alter device selection.
		virtual void BeforeDeviceBuilding(vkb::DeviceBuilder& deviceBuilder) {};

		/// @brief Base init is heavily overriden by this class, because a complete simple vulkan setup is included.
		virtual void BaseInit() override;
		
		/// @brief The main window used for rendering.
		Window mWindow;

#pragma region Vulkan
		VkSurfaceKHR mSurface{};

		vkb::PhysicalDevice mVkbPhysicalDevice;
		VkPhysicalDevice mPhysicalDevice{};

		vkb::Device mVkbDevice;
		VkDevice mDevice{};
#pragma endregion
	};
}