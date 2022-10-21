#pragma once
#include "../foray_basics.hpp"
#include <functional>
#include "../foray_vkb.hpp"

namespace foray::base {
    /// @brief Wraps creation and lifetime of a vulkan instance. Includes default debug callback logging setup.
    class VulkanInstance
    {
      public:
        /// @brief Function called after default callback configuration and before action of the instance builder
        using BeforeInstanceBuildFunctionPointer = std::function<void(vkb::InstanceBuilder&)>;

        VulkanInstance() = default;
        /// @param beforeInstanceBuildFunc Function called after default callback configuration and before action of the instance builder
        /// @param enableDebugLayersAndCallbacks If true, validation layers are enabled, and if mDebugCallbackFunc isn't null sets debug layers callback
        inline VulkanInstance(core::Context* context, BeforeInstanceBuildFunctionPointer beforeInstanceBuildFunc, bool enableDebugLayersAndCallbacks)
            : mBeforeInstanceBuildFunc{beforeInstanceBuildFunc}, mEnableDebugLayersAndCallbacks{enableDebugLayersAndCallbacks}, mContext{context}
        {
        }

        /// @brief Set the function called after default callback configuration and before action of the instance builder
        VulkanInstance& SetBeforeInstanceBuildFunc(BeforeInstanceBuildFunctionPointer beforeInstanceBuildFunc);
        /// @brief Set the function receiving vulkan validation messages
        VulkanInstance& SetDebugCallbackFunc(PFN_vkDebugUtilsMessengerCallbackEXT debugCallbackFunc);
        
        FORAY_PROPERTY_ALL(DebugUserData)
        FORAY_PROPERTY_ALL(EnableDebugLayersAndCallbacks)
        FORAY_PROPERTY_ALL(Instance)
        FORAY_PROPERTY_ALL(Context)

        inline operator bool() const { return !!mInstance.instance; }
        inline operator VkInstance() const { return mInstance.instance; }

        /// @brief If mEnableDebugLayersAndCallbacks is set, configures validation and callback. If mBeforeInstanceBuildFunc is set, calls it. Finally, builds Instance.
        /// @remark Will throw std::exception on instance build failure
        void        Create();
        inline bool Exists() const { return !!mInstance.instance; }
        void        Destroy();

        ~VulkanInstance();

      protected:
        static VkBool32 DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                             VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                             void*                                       pUserData);

        BeforeInstanceBuildFunctionPointer   mBeforeInstanceBuildFunc = nullptr;
        PFN_vkDebugUtilsMessengerCallbackEXT mDebugCallbackFunc       = &DefaultDebugCallback;
        void*                                mDebugUserData           = nullptr;

        /// @brief If true, validation layers are enabled, and if mDebugCallbackFunc or mDebugUserData are set they are passed on to the builder respectively
        bool mEnableDebugLayersAndCallbacks = true;

        core::Context* mContext = nullptr;

        vkb::Instance mInstance;
    };
}  // namespace foray::base
