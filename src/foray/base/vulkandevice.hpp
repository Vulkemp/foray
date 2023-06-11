#pragma once
#include "../basics.hpp"
#include "../vkb.hpp"
#include <functional>

namespace foray::base {
    /// @brief Wraps selection and creation of a vulkan logical device
    class VulkanDevice
    {
      public:
        /// @brief Function called after default configuration and before action with physical device selector
        using BeforePhysicalDeviceSelectFunctionPointer = std::function<void(vkb::PhysicalDeviceSelector&)>;
        /// @brief Function called after default configuration and before action with device builder
        using BeforeDeviceBuildFunctionPointer = std::function<void(vkb::DeviceBuilder&)>;

        VulkanDevice() = default;
        /// @param beforePhysicalDeviceSelectFunc Function called after default configuration and before action with physical device selector
        /// @param beforeDeviceBuildFunc Function called after default configuration and before action with device builder
        inline VulkanDevice(core::Context*                            context,
                            BeforePhysicalDeviceSelectFunctionPointer beforePhysicalDeviceSelectFunc,
                            BeforeDeviceBuildFunctionPointer          beforeDeviceBuildFunc)
            : mBeforePhysicalDeviceSelectFunc{beforePhysicalDeviceSelectFunc}, mBeforeDeviceBuildFunc{beforeDeviceBuildFunc}, mContext{context}
        {
        }

        inline operator bool() const { return !!mDevice.device; }
        inline operator VkDevice() const { return mDevice.device; }
        inline operator VkPhysicalDevice() const { return mPhysicalDevice.physical_device; }
        inline operator vkb::Device&() { return mDevice; }
        inline operator const vkb::Device&() const { return mDevice; }

        /// @brief Set the function called after default configuration and before action with physical device selector
        VulkanDevice& SetBeforePhysicalDeviceSelectFunc(BeforePhysicalDeviceSelectFunctionPointer beforePhysicalDeviceSelectFunc);
        /// @brief Set the function called after default configuration and before action with device builder
        VulkanDevice& SetBeforeDeviceBuildFunc(BeforeDeviceBuildFunctionPointer beforeDeviceBuildFunc);

        FORAY_PROPERTY_V(EnableDefaultFeaturesAndExtensions)
        FORAY_PROPERTY_V(EnableRaytracingFeaturesAndExtensions)
        FORAY_PROPERTY_V(ShowConsoleDeviceSelectionPrompt)
        FORAY_PROPERTY_R(PhysicalDevice)
        FORAY_PROPERTY_R(Device)
        FORAY_PROPERTY_R(DispatchTable)
        FORAY_PROPERTY_V(Context)
        FORAY_GETTER_R(Properties)

        /// @brief Create logical device by invoking SelectPhysicalDevice(..,) and BuildDevice()
        /// @remark Will throw std::exception on selection or build failure
        void Create();
        /// @brief If mSetDefaultCapabilitiesToDeviceSelector is set, configures defaults. If mBeforePhysicalDeviceSelectFunc is set, invokes it. Finally, selects device.
        /// @remark Will throw std::exception if no selection could be made
        void SelectPhysicalDevice();
        /// @brief If mEnableDefaultDeviceFeatures is set, configures defaults. If mBeforeDeviceBuildFunc is set, invokes it. Builds device.
        /// @remark Will throw std::exception if building fails
        void        BuildDevice();
        inline bool Exists() const { return !!mDevice.device; }

        virtual ~VulkanDevice();

        struct Properties
        {
            VkPhysicalDeviceProperties2                        Properties2;
            VkPhysicalDeviceAccelerationStructurePropertiesKHR AsProperties;
        };

      protected:
        BeforePhysicalDeviceSelectFunctionPointer mBeforePhysicalDeviceSelectFunc = nullptr;
        BeforeDeviceBuildFunctionPointer          mBeforeDeviceBuildFunc          = nullptr;

        /// @brief Configures device selector with default extensions and features
        /// @details
        ///   - Requires present capability, prefers dedicated devices.
        ///   - Enables VK_KHR_BUFFER_DEVICE_ADDRESS, VK_EXT_DESCRIPTOR_INDEXING, VK_KHR_SPIRV_1_4, VK_KHR_RELAXED_BLOCK_LAYOUT, VK_KHR_SYNCHRONIZATION_2
        ///         extensions (plus extensions those depend on).
        ///   - Enables samplerAnisotropy feature.
        ///   - Enables BufferDeviceAddress, DescriptorIndexing and Synchronization2 features
        bool mEnableDefaultFeaturesAndExtensions = true;
        /// @brief Configures device selector with default extensions and features
        /// @details
        ///   - Implicitly sets mEnableDefaultFeaturesAndExtensions to true
        ///   - Enables VK_KHR_ACCELERATION_STRUCTURE and VK_KHR_RAY_TRACING_PIPELINE extensions (plus extensions those depend on)
        ///   - Enables RayTracingPipelines and AccelerationStructures features
        bool mEnableRaytracingFeaturesAndExtensions = true;
        /// @brief If enabled, prompts the user in the console to select a device if multiple suitable devices are present. If disabled, selects the first index.
        bool mShowConsoleDeviceSelectionPrompt = false;

        core::Context* mContext = nullptr;

        vkb::PhysicalDevice mPhysicalDevice;
        vkb::Device         mDevice;
        vkb::DispatchTable  mDispatchTable;

        struct
        {
            VkPhysicalDeviceBufferDeviceAddressFeatures      BufferDeviceAdressFeatures    = {};
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR    RayTracingPipelineFeatures    = {};
            VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures = {};
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT    DescriptorIndexingFeatures    = {};
        } mFeatures = {};

        Properties mProperties = {};
    };
}  // namespace foray::base