#include "vulkandevice.hpp"
#include "../core/context.hpp"
#include "../logger.hpp"
#include "../osi/window.hpp"
#include "../vulkan.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

namespace foray::base {
    VulkanDevice::Selector::Selector(Ref<IVulkanInstance> instance, View<osi::Window> window)
        : Instance(instance), Surface(!!window ? window->GetOrCreateSurfaceKHR(instance) : nullptr)
    {
        std::vector<vk::PhysicalDevice> devices = Instance->Get().enumeratePhysicalDevices();

        for(vk::PhysicalDevice device : devices)
        {
            Adapters.push_back(device);
        }
    }

    std::vector<Ref<const VulkanAdapterInfo>> VulkanDevice::Selector::GetCompatibleAdapters(bool logDecisionMaking)
    {
        std::span<const VulkanAdapterInfo>        adapters = Instance->GetAdapterInfos();
        std::vector<Ref<const VulkanAdapterInfo>> result;
        std::stringstream                         strbuilder;
        strbuilder << "Found " << adapters.size() << " installed Vulkan Device(s):\n";
        for(const VulkanAdapterInfo& info : adapters)
        {
            strbuilder
                << fmt::format("[{:x}] {} ({})", info.Properties.properties.deviceID, info.Properties.properties.deviceName, NAMEOF_ENUM(info.Properties.properties.deviceType));
            result.push_back(&info);
        }

        if(logDecisionMaking)
        {
            logger()->info(strbuilder.str());
        }
        strbuilder.clear();

        //

        return result;
    }

    Ref<const VulkanAdapterInfo> VulkanDevice::Selector::SelectCompatibleAdapterAsserted(bool logDecisionMaking, bool questionUser)
    {
        std::vector<Ref<const VulkanAdapterInfo>> adapters = GetCompatibleAdapters(logDecisionMaking);
        Assert(!adapters.empty(), "No suitable device available!");

        uint32_t          selectIndex = 0;
        if(questionUser && adapters.size() > 1)
        {
            std::stringstream strbuilder;
            strbuilder << "Device Selector has detected " << ret->size() << " suitable devices:\n";
            for(int32_t index = 0; index < (int32_t)ret->size(); index++)
            {
                strbuilder << std::setw(3) << index << "  " << ret->at(index).name << "\n";
            }
            strbuilder << "Type a number in the range [0..." << (ret->size() - 1) << "]: ";
            std::cout << strbuilder.str() << std::flush;
            char in[256] = {};
            std::cin >> std::setw(255) >> in;
            selectIndex = (uint32_t)strtoul(in, nullptr, 10);
            if(selectIndex >= ret->size())
            {
                selectIndex = 0;
            }
        }
        logger()->info("Selecting device [{:x}] {} ({})", adapters[selectIndex]->Properties.properties.deviceID, adapters[selectIndex]->Properties.properties.deviceName, NAMEOF_ENUM(adapters[selectIndex]->Properties.properties.deviceType));
        return adapters[selectIndex];
    }

    VulkanDevice& VulkanDevice::SetBeforePhysicalDeviceSelectFunc(BeforePhysicalDeviceSelectFunctionPointer beforePhysicalDeviceSelectFunc)
    {
        mBeforePhysicalDeviceSelectFunc = beforePhysicalDeviceSelectFunc;
        return *this;
    }
    VulkanDevice& VulkanDevice::SetBeforeDeviceBuildFunc(BeforeDeviceBuildFunctionPointer beforeDeviceBuildFunc)
    {
        mBeforeDeviceBuildFunc = beforeDeviceBuildFunc;
        return *this;
    }

    void VulkanDevice::Create()
    {
        SelectPhysicalDevice();
        BuildDevice();
    }
    void VulkanDevice::SelectPhysicalDevice()
    {
        Assert(!!(mContext->Instance), "[VulkanDevice::SelectPhysicalDevice] require instance to initialize device selection process!");

        // create physical device selector
        vkb::PhysicalDeviceSelector deviceSelector(mContext->Instance->GetInstance(), mContext->WindowSwapchain->GetOrCreateSurface());

        std::vector<std::string> availableDevices = deviceSelector.select_device_names().value();

        if(mEnableRaytracingFeaturesAndExtensions)
        {
            mEnableDefaultFeaturesAndExtensions = true;
        }

        if(mEnableDefaultFeaturesAndExtensions)
        {
            // Require capability to present to the current windows surface
            deviceSelector.require_present();

            // Prefer dedicated devices
            deviceSelector.prefer_gpu_device_type();

            deviceSelector.set_minimum_version(1U, 3U);

            // Set raytracing extensions
            std::vector<const char*> requiredExtensions{
                VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
                VK_KHR_SPIRV_1_4_EXTENSION_NAME,
                // Relaxed block layout allows custom strides for buffer layouts. Used for index buffer and vertex buffer in rt shaders
                VK_KHR_RELAXED_BLOCK_LAYOUT_EXTENSION_NAME,
            };
            deviceSelector.add_required_extensions(requiredExtensions);

            // Enable samplerAnisotropy
            VkPhysicalDeviceFeatures deviceFeatures{};
            deviceFeatures.samplerAnisotropy = VK_TRUE;

            deviceSelector.set_required_features(deviceFeatures);
        }

        {  // Enable dynamic rendering and sync2
            VkPhysicalDeviceVulkan13Features deviceFeatures{};
            deviceFeatures.dynamicRendering = VK_TRUE;
            deviceFeatures.synchronization2 = VK_TRUE;
            deviceSelector.set_required_features_13(deviceFeatures);
        }


        if(mEnableRaytracingFeaturesAndExtensions)
        {
            // Set raytracing extensions
            std::vector<const char*> requiredExtensions{VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,  // acceleration structure
                                                        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,  // rt pipeline
                                                        // dependencies of acceleration structure
                                                        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                                                        // dependencies of rt pipeline
                                                        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME};
            deviceSelector.add_required_extensions(requiredExtensions);
        }

        if(!!mBeforePhysicalDeviceSelectFunc)
        {
            mBeforePhysicalDeviceSelectFunc(deviceSelector);
        }

        auto ret = deviceSelector.select_devices();
        if(ret.has_value() && ret->size() > 0)
        {
            uint32_t selectIndex = 0;
            if(ret->size() > 1 && mShowConsoleDeviceSelectionPrompt)
            {
                std::stringstream strbuilder;
                strbuilder << "Device Selector has detected " << ret->size() << " suitable devices:\n";
                for(int32_t index = 0; index < (int32_t)ret->size(); index++)
                {
                    strbuilder << std::setw(3) << index << "  " << ret->at(index).name << "\n";
                }
                strbuilder << "Type a number in the range [0..." << (ret->size() - 1) << "]: ";
                std::cout << strbuilder.str() << std::flush;
                char in[256] = {};
                std::cin >> std::setw(255) >> in;
                selectIndex = (uint32_t)strtoul(in, nullptr, 10);
                if(selectIndex >= ret->size())
                {
                    selectIndex = 0;
                }
            }
            logger()->info("Device Selector chooses \"{}\"", ret->at(selectIndex).name);
            mPhysicalDevice  = ret->at(selectIndex);
            mContext->Device = this;

            mProperties.Properties2 = VkPhysicalDeviceProperties2{.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
            mProperties.AsProperties =
                VkPhysicalDeviceAccelerationStructurePropertiesKHR{.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};

            if(mEnableRaytracingFeaturesAndExtensions)
            {
                mProperties.Properties2.pNext = &mProperties.AsProperties;
            }

            vkGetPhysicalDeviceProperties2(mPhysicalDevice, &mProperties.Properties2);
        }
        else
        {
            logger()->warn("Device Selector failed to find suitable device!");
            FORAY_THROWFMT("[VulkanDevice::SelectPhysicalDevice] vkb Device Selector failed to find a satisifying device. vk::Result: {} Reason: {}",
                           PrintVkResult(ret.vk_result()), ret.error().message())
        }
    }
    void VulkanDevice::BuildDevice()
    {
        Assert(!!mPhysicalDevice.physical_device, "[VulkanDevice::BuildDevice] Must select physical device first!");

        vkb::DeviceBuilder deviceBuilder(mPhysicalDevice);

        mFeatures.BufferDeviceAdressFeatures = {.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES, .bufferDeviceAddress = VK_TRUE};

        mFeatures.RayTracingPipelineFeatures = {.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR, .rayTracingPipeline = VK_TRUE};

        mFeatures.AccelerationStructureFeatures = {.sType                 = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
                                                   .accelerationStructure = VK_TRUE};

        mFeatures.DescriptorIndexingFeatures = {.sType                                     = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
                                                .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
                                                .runtimeDescriptorArray                    = VK_TRUE};  // enable this for unbound descriptor arrays

        if(mEnableDefaultFeaturesAndExtensions)
        {
            deviceBuilder.add_pNext(&mFeatures.BufferDeviceAdressFeatures);
            deviceBuilder.add_pNext(&mFeatures.DescriptorIndexingFeatures);
        }
        if(mEnableRaytracingFeaturesAndExtensions)
        {
            deviceBuilder.add_pNext(&mFeatures.RayTracingPipelineFeatures);
            deviceBuilder.add_pNext(&mFeatures.AccelerationStructureFeatures);
        }

        if(!!mBeforeDeviceBuildFunc)
        {
            mBeforeDeviceBuildFunc(deviceBuilder);
        }

        auto ret = deviceBuilder.build();
        FORAY_ASSERTFMT(ret.has_value(), "[VulkanDevice::BuildDevice] vkb Device Builder failed to build device. vk::Result: {} Reason: {}", PrintVkResult(ret.vk_result()),
                        ret.error().message())

        mDevice        = *ret;
        mDispatchTable = mDevice.make_table();
    }

    VulkanDevice::~VulkanDevice()
    {
        if(!!mDevice.device)
        {
            vkb::destroy_device(mDevice);
            mDevice = vkb::Device();
        }
        mPhysicalDevice = vkb::PhysicalDevice();
        mDispatchTable  = vkb::DispatchTable();
        if(!!mContext)
        {
            mContext->Device = nullptr;
        }
    }

}  // namespace foray::base