#include "adapter.hpp"

namespace foray::base
{
    VulkanAdapterInfo::VulkanAdapterInfo(vk::PhysicalDevice device) : Device(device)
    {
        // Get Features
        Features.pNext                   = &Vulkan11Features;
        Vulkan11Features.pNext           = &Vulkan12Features;
        Vulkan12Features.pNext           = &Vulkan13Features;
        Vulkan13Features.pNext           = &RayTracingPipelineFeatures;
        RayTracingPipelineFeatures.pNext = &AccelerationStructureFeatures;

        Device.getFeatures2(&Features);

        // Get Properties
        Properties.pNext         = &Vulkan11Properties;
        Vulkan11Properties.pNext = &Vulkan12Properties;
        Vulkan12Properties.pNext = &Vulkan13Properties;
        Vulkan13Properties.pNext = &AccelerationStructureProperties;

        Device.getProperties2(&Properties);

        // Get Extensions
        const std::vector<vk::ExtensionProperties> extensionsProperties = Device.enumerateDeviceExtensionProperties();
        for(const vk::ExtensionProperties& extensionProperties : extensionsProperties)
        {
            Extensions.Add((const char*)extensionProperties.extensionName);
        }

        // Get Layers
        const std::vector<vk::LayerProperties> layersProperties = Device.enumerateDeviceLayerProperties();
        for(const vk::LayerProperties& layerProperties : layersProperties)
        {
            Layers.Add((const char*)layerProperties.layerName);
        }
    }
} // namespace foray::base
