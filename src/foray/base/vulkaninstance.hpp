#pragma once
#include "../basics.hpp"
#include "../osi/osi_declares.hpp"
#include "../vulkan.hpp"
#include "initialization.hpp"
#include "interface.hpp"
#include "../util/stringset.hpp"
#include <functional>
#include <unordered_set>

namespace foray::base {


    /// @brief Wraps creation and lifetime of a vulkan instance. Includes default debug callback logging setup.
    class VulkanInstance : public IVulkanInstance, public NoMoveDefaults
    {
      public:
        struct LayerNames
        {
            inline static const char* Validation = "VK_LAYER_KHRONOS_validation";
        };

        class CreateInfo
        {
          public:
            CreateInfo() = default;
            /// @brief Set default values:
            ///  * appInfo engine fields,
            ///  * enables validation & debug printf,
            ///  * Vulkan 1.2 required, 1.3 desired
            /// @param window OPTIONAL: also sets Surface extensions
            void SetDefaults(View<osi::Window> window);

            FORAY_PROPERTY_R(AppInfo)
            FORAY_PROPERTY_R(EnabledExtensions)
            FORAY_PROPERTY_R(EnabledLayers)
            FORAY_PROPERTY_V(RequiredApiVersion)
            FORAY_PROPERTY_V(DesiredApiVersion)
            FORAY_PROPERTY_R(ValidationFeatureEnables)
          protected:
            vk::ApplicationInfo                                mAppInfo;
            util::StringSet                                    mEnabledExtensions;
            util::StringSet                                    mEnabledLayers;
            uint32_t                                           mRequiredApiVersion = VK_API_VERSION_1_0;
            uint32_t                                           mDesiredApiVersion  = VK_API_VERSION_1_0;
            std::unordered_set<vk::ValidationFeatureEnableEXT> mValidationFeatureEnables;
        };

        /// @brief Populates the StringSet input with all extensions available 
        /// to the local Vulkan implementation
        static void GetAvailableExtensions(Ref<util::StringSet> out_extensions);
        /// @brief Populates the StringSet input with all layers available 
        /// to the local Vulkan implementation
        static void GetAvailableLayers(Ref<util::StringSet> out_layers);

        // /// @brief Initializes Vulkan.hpp
        // static void InitializeVulkanHpp();

        /// @brief Instantiates a Vulkan Instance. Creates:
        ///  * vk::Instance
        ///  * Debug Messenger Callback
        ///  * Debug Report Callback
        VulkanInstance(const CreateInfo& ci);

        FORAY_PROPERTY_V(ThrowOnValidationError)

        /// @brief Finalizes the Vulkan Instance
        virtual ~VulkanInstance();
        virtual vk::Instance       Get() override;
        virtual const vk::Instance Get() const override;

        virtual std::span<const VulkanAdapterInfo> GetAdapterInfos() const override;

        virtual VkBool32 OnDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                  VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);

        virtual VkBool32 OnDebugReportCallback(VkDebugReportFlagsEXT      flags,
                                            VkDebugReportObjectTypeEXT objectType,
                                            uint64_t                   object,
                                            size_t                     location,
                                            int32_t                    messageCode,
                                            const char*                pLayerPrefix,
                                            const char*                pMessage);
      protected:

        bool mThrowOnValidationError = true;

        vk::Instance mInstance;
        vk::DebugUtilsMessengerEXT mDebugMessenger;
        vk::DebugReportCallbackEXT mDebugReport;
        std::vector<VulkanAdapterInfo> mAdapters;
        util::StringSet mExtensions;
        util::StringSet mLayers;
    };
}  // namespace foray::base
