#pragma once
#include "../foray_basics.hpp"
#include "../foray_vkb.hpp"
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
        inline VulkanDevice(core::Context* context, BeforePhysicalDeviceSelectFunctionPointer beforePhysicalDeviceSelectFunc, BeforeDeviceBuildFunctionPointer beforeDeviceBuildFunc)
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

        FORAY_PROPERTY_V(SetDefaultCapabilitiesToDeviceSelector)
        FORAY_PROPERTY_V(EnableDefaultDeviceFeatures)
        FORAY_PROPERTY_R(PhysicalDevice)
        FORAY_PROPERTY_R(Device)
        FORAY_PROPERTY_R(DispatchTable)
        FORAY_PROPERTY_V(Context)

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
        void        Destroy();

        ~VulkanDevice();

      protected:
        BeforePhysicalDeviceSelectFunctionPointer mBeforePhysicalDeviceSelectFunc = nullptr;
        BeforeDeviceBuildFunctionPointer          mBeforeDeviceBuildFunc          = nullptr;

        /// @brief Requires present capability, prefers dedicated devices. Enables VK_KHR_ACCELERATION_STRUCTURE, VK_KHR_RAY_TRACING_PIPELINE and VK_KHR_SYNCHRONIZATION_2 extensions (plus extensions those depend on). Enables samplerAnisotropy feature.
        bool mSetDefaultCapabilitiesToDeviceSelector = true;
        /// @brief Enables features listed in mDefaultFeatures member
        bool mEnableDefaultDeviceFeatures = true;

        core::Context* mContext = nullptr;

        vkb::PhysicalDevice mPhysicalDevice;
        vkb::Device         mDevice;
        vkb::DispatchTable  mDispatchTable;

        struct DefaultFeatures
        {
            VkPhysicalDeviceBufferDeviceAddressFeatures      BufferDeviceAdressFeatures    = {};
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR    RayTracingPipelineFeatures    = {};
            VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures = {};
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT    DescriptorIndexingFeatures    = {};
            VkPhysicalDeviceSynchronization2Features         Sync2FEatures                 = {};
        } mDefaultFeatures = {};
    };
}  // namespace foray::base