#pragma once
#include "../foray_basics.hpp"
#include "../foray_vkb.hpp"
#include <functional>

namespace foray::base {
    /// @brief Wraps creation and lifetime of a vulkan instance. Includes default debug callback logging setup.
    class VulkanInstance
    {
      public:
        /// @brief Function called after default callback configuration and before action of the instance builder
        using BeforeInstanceBuildFunctionPointer = std::function<void(vkb::InstanceBuilder&)>;

        VulkanInstance() = default;
        /// @param beforeInstanceBuildFunc Function called after default callback configuration and before action of the instance builder
        /// @param enableDebugLayersAndCallbacks If true, validation layers are enabled, and if mDebugMessengerFunc isn't null sets debug layers callback
        inline VulkanInstance(core::Context* context, BeforeInstanceBuildFunctionPointer beforeInstanceBuildFunc, bool enableDebugLayersAndCallbacks)
            : mBeforeInstanceBuildFunc{beforeInstanceBuildFunc}, mEnableDebugLayersAndCallbacks{enableDebugLayersAndCallbacks}, mContext{context}
        {
        }

        /// @brief Set the function called after default callback configuration and before action of the instance builder
        FORAY_SETTER_V(BeforeInstanceBuildFunc);
        /// @brief Set the function receiving vulkan validation messages
        FORAY_SETTER_V(DebugMessengerFunc);
        FORAY_SETTER_V(DebugReportFunc);

        FORAY_PROPERTY_V(DebugUserData)
        FORAY_PROPERTY_V(EnableDebugLayersAndCallbacks)
        FORAY_PROPERTY_V(EnableDebugReport)
        FORAY_PROPERTY_V(ThrowOnValidationError)
        FORAY_PROPERTY_R(Instance)
        FORAY_PROPERTY_V(Context)

        inline operator bool() const { return !!mInstance.instance; }
        inline operator VkInstance() const { return mInstance.instance; }

        /// @brief If mEnableDebugLayersAndCallbacks is set, configures validation and callback. If mBeforeInstanceBuildFunc is set, calls it. Finally, builds Instance.
        /// @remark Will throw std::exception on instance build failure
        void        Create();

        virtual ~VulkanInstance();

      protected:
        static VkBool32 DefaultDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                      VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                      void*                                       pUserData);

        static VkBool32 DefaultDebugReportCallback(VkDebugReportFlagsEXT      flags,
                                                   VkDebugReportObjectTypeEXT objectType,
                                                   uint64_t                   object,
                                                   size_t                     location,
                                                   int32_t                    messageCode,
                                                   const char*                pLayerPrefix,
                                                   const char*                pMessage,
                                                   void*                      pUserData);

        BeforeInstanceBuildFunctionPointer   mBeforeInstanceBuildFunc   = nullptr;
        PFN_vkDebugUtilsMessengerCallbackEXT mDebugMessengerFunc        = &DefaultDebugMessengerCallback;
        void*                                mDebugUserData             = nullptr;
        PFN_vkDebugReportCallbackEXT         mDebugReportFunc           = &DefaultDebugReportCallback;
        VkDebugReportCallbackEXT             mDebugReportCallbackHandle = nullptr;

        /// @brief If true, validation layers are enabled, and if mDebugMessengerFunc or mDebugUserData are set they are passed on to the builder respectively
        bool mEnableDebugLayersAndCallbacks = true;

        bool mEnableDebugReport = false;

        bool mThrowOnValidationError = true;

        core::Context* mContext = nullptr;

        vkb::Instance mInstance;
    };
}  // namespace foray::base
