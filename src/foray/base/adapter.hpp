#pragma once
#include "../util/stringset.hpp"
#include "../vulkan.hpp"

namespace foray::base {
    class VulkanAdapterInfo
    {
      public:
        VulkanAdapterInfo() = default;
        VulkanAdapterInfo(vk::PhysicalDevice device);

        vk::PhysicalDevice                                   Device;
        util::StringSet                                      Extensions;
        util::StringSet                                      Layers;
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
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR AccelerationStructureProperties;
    };
}  // namespace foray::base
