#pragma once
#include "../basics.hpp"
#include "../osi/osi_declares.hpp"
#include "../util/stringset.hpp"
#include "initialization.hpp"
#include "interface.hpp"
#include <functional>

namespace foray::base {
    /// @brief Wraps selection and creation of a vulkan logical device
    class VulkanDevice : public IVulkanDevice, public NoMoveDefaults
    {
      public:
        class CreateInfo
        {
          public:
            CreateInfo(Ref<const VulkanAdapterInfo> adapter);

            util::StringSet Extensions;
        };

        class Selector
        {
          public:
            Selector(Ref<IVulkanInstance> instance, View<osi::Window> window);
            std::vector<Ref<const VulkanAdapterInfo>> GetCompatibleAdapters(bool logDecisionMaking);

            Ref<const VulkanAdapterInfo> SelectCompatibleAdapterAsserted(bool logDecisionMaking, bool questionUser);

            Ref<IVulkanInstance>                                 Instance;
            util::StringSet                                      RequiredExtensions;
            util::StringSet                                      DesiredExtensions;
            vk::PhysicalDeviceFeatures2                          Features;
            vk::PhysicalDeviceVulkan11Features                   Vulkan11Features;
            vk::PhysicalDeviceVulkan12Features                   Vulkan12Features;
            vk::PhysicalDeviceVulkan13Features                   Vulkan13Features;
            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR      RayTracingPipelineFeatures;
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR   AccelerationStructureFeatures;
            vk::PhysicalDeviceProperties2                        Properties;
            vk::PhysicalDeviceVulkan11Properties                 Vulkan11Properties;
            vk::PhysicalDeviceVulkan12Properties                 Vulkan12Properties;
            vk::PhysicalDeviceVulkan13Properties                 Vulkan13Properties;
            vk::PhysicalDeviceAccelerationStructurePropertiesKHR AsProperties;
            EFeatureSwitch                                       RayTracing = EFeatureSwitch::Required;
            vk::SurfaceKHR                                       Surface    = nullptr;
        };

        inline VulkanDevice(Ref<core::Context> context, Ref<const VulkanAdapterInfo> adapter);
        inline VulkanDevice(Ref<IVulkanInstance> instance, Ref<const VulkanAdapterInfo> adapter);

        inline operator vk::Device() const { return mDevice; }
        inline operator vk::PhysicalDevice() const { return mAdapterInfo->Device; }

        virtual ~VulkanDevice();

      protected:
        View<core::Context> mContext = nullptr;
        Ref<const VulkanAdapterInfo> mAdapterInfo;

        vk::Device             mDevice;
    };
}  // namespace foray::base