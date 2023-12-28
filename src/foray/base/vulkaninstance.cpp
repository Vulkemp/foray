#include "vulkaninstance.hpp"
#include "../core/context.hpp"
#include "../exception.hpp"
#include "../logger.hpp"
#include "../osi/window.hpp"
#include "../util/stringset.hpp"
#include "../vulkan.hpp"

VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                                           VkDebugUtilsMessageTypeFlagsEXT             message_type,
                                                           const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                           void*                                       user_data)
{
    reinterpret_cast<foray::base::VulkanInstance*>(user_data)->OnDebugMessengerCallback(message_severity, message_type, callback_data);
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT      flags,
                                                   VkDebugReportObjectTypeEXT objectType,
                                                   uint64_t                   object,
                                                   size_t                     location,
                                                   int32_t                    messageCode,
                                                   const char*                pLayerPrefix,
                                                   const char*                pMessage,
                                                   void*                      user_data)
{
    reinterpret_cast<foray::base::VulkanInstance*>(user_data)->OnDebugReportCallback(flags, objectType, object, location, messageCode, pLayerPrefix, pMessage);
}

namespace foray::base {


    void VulkanInstance::GetAvailableExtensions(Ref<util::StringSet> out_extensions)
    {
        std::vector<vk::ExtensionProperties> extensionsProperties = vk::enumerateInstanceExtensionProperties();
        for(vk::ExtensionProperties& extensionProperties : extensionsProperties)
        {
            out_extensions->Add((const char*)extensionProperties.extensionName);
        }
    }

    void VulkanInstance::GetAvailableLayers(Ref<util::StringSet> out_layers)
    {
        std::vector<vk::LayerProperties> layersProperties = vk::enumerateInstanceLayerProperties();
        for(vk::LayerProperties& layerProperties : layersProperties)
        {
            out_layers->Add((const char*)layerProperties.layerName);
        }
    }

    // void VulkanInstance::InitializeVulkanHpp()
    // {
    // }

    VulkanInstance::VulkanInstance(const CreateInfo& ci)
    {
        vk::ApplicationInfo appInfo = ci.GetAppInfo();
        if(appInfo.apiVersion == 0)
        {
            uint32_t userApiVersion = ci.GetRequiredApiVersion();
            FORAY_ASSERTFMT(userApiVersion <= vk::ApiVersion, "Required Api version {:x} greater than instance version {:x}", ci.GetRequiredApiVersion(), vk::ApiVersion)

            if(ci.GetDesiredApiVersion() != 0)
            {
                if(ci.GetDesiredApiVersion() <= vk::ApiVersion)
                {
                    userApiVersion = ci.GetDesiredApiVersion();
                }
                else
                {
                    logger()->warn("Desired Api Version {:x} not supported by instance!", ci.GetDesiredApiVersion());
                }
            }
            if(userApiVersion < vk::ApiVersion10)
            {
                userApiVersion = vk::ApiVersion10;
            }
            appInfo.apiVersion = userApiVersion;
        }

        vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCi({}, vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
                                                              vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                                                              DebugUtilsMessengerCallback);

        vk::DebugReportCallbackCreateInfoEXT debugReportCi(vk::DebugReportFlagBitsEXT::eInformation, DebugReportCallback, this);

        util::StringSet AvailableLayers;
        util::StringSet AvailableExtensions;
        GetAvailableLayers(AvailableLayers);
        GetAvailableExtensions(AvailableExtensions);

        std::vector<const char*> enabledLayers;
        enabledLayers.reserve(ci.GetEnabledLayers().GetCount());
        for(const std::string_view layer : ci.GetEnabledLayers())
        {
            enabledLayers.push_back(layer.data());
            FORAY_ASSERTFMT(AvailableLayers.Has(layer), "Requested layer \"{}\" is not supported", layer)
        }

        std::vector<const char*> enabledExtensions;
        enabledExtensions.reserve(ci.GetEnabledExtensions().GetCount());
        for(const std::string_view extension : ci.GetEnabledExtensions())
        {
            enabledExtensions.push_back(extension.data());
            FORAY_ASSERTFMT(AvailableLayers.Has(extension), "Requested extension \"{}\" is not supported", extension)
        }

        vk::InstanceCreateInfo vkCi({}, &appInfo, enabledLayers, enabledExtensions);
        vkCi.setPNext(&debugMessengerCi);
        debugMessengerCi.setPNext(&debugReportCi);

        mInstance       = vk::createInstance(vkCi);
        mDebugMessenger = mInstance.createDebugUtilsMessengerEXT(debugMessengerCi);
        mDebugReport    = mInstance.createDebugReportCallbackEXT(debugReportCi);

        for (vk::PhysicalDevice physicalDevice : mInstance.enumeratePhysicalDevices())
        {
            mAdapters.push_back(VulkanAdapterInfo(physicalDevice));
        }
    }

    // void VulkanInstance::Create()
    // {
    //     vkb::InstanceBuilder instanceBuilder;
    //     if(mEnableDebugLayersAndCallbacks)
    //     {
    //         instanceBuilder.enable_validation_layers();
    //         if(!!mDebugMessengerFunc)
    //         {
    //             instanceBuilder.set_debug_callback(mDebugMessengerFunc);
    //         }
    //         if(mDebugMessengerFunc == &DefaultDebugMessengerCallback)
    //         {
    //             instanceBuilder.set_debug_callback_user_data_pointer(this);
    //         }
    //         else if(!!mDebugUserData)
    //         {
    //             instanceBuilder.set_debug_callback_user_data_pointer(mDebugUserData);
    //         }
    //     }
    //     if(mEnableDebugReport)
    //     {
    //         instanceBuilder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
    //         instanceBuilder.enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    //         instanceBuilder.enable_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    //     }
    //     instanceBuilder.require_api_version(VK_MAKE_API_VERSION(0, 1, 3, 0));
    //     instanceBuilder.set_minimum_instance_version(VK_MAKE_API_VERSION(0, 1, 3, 0));
    //     if(!!mContext && !!mContext->WindowSwapchain)
    //     {
    //         std::vector<const char*> surfaceExtensions = mContext->WindowSwapchain->GetWindow().GetVkSurfaceExtensions();
    //         for(const char* ext : surfaceExtensions)
    //         {
    //             instanceBuilder.enable_extension(ext);
    //         }
    //     }
    //     if(!!mBeforeInstanceBuildFunc)
    //     {
    //         mBeforeInstanceBuildFunc(instanceBuilder);
    //     }
    //     auto ret = instanceBuilder.build();
    //     FORAY_ASSERTFMT(ret.has_value(), "[VulkanInstance::Create] vkb Instance Builder failed to build instance. vk::Result: {} Reason: {}", PrintVkResult(ret.vk_result()),
    //                     ret.error().message())
    //     mInstance = *ret;
    //     if(!!mContext)
    //     {
    //         mContext->Instance = this;
    //     }
    //     if(mEnableDebugReport)
    //     {
    //         // Populate the VkDebugReportCallbackCreateInfoEXT
    //         VkDebugReportCallbackCreateInfoEXT ci = {};
    //         ci.sType                              = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    //         ci.pfnCallback                        = mDebugReportFunc;
    //         ci.flags                              = VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
    //         ci.pUserData                          = nullptr;
    //         PFN_vkCreateDebugReportCallbackEXT createDebugReportCallback = VK_NULL_HANDLE;
    //         createDebugReportCallback                                    = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugReportCallbackEXT");
    //         // Create the callback handle
    //         createDebugReportCallback(mContext->vk::Instance(), &ci, nullptr, &mDebugReportCallbackHandle);
    //     }
    // }

    VulkanInstance::~VulkanInstance()
    {
        if(!!mDebugMessenger)
        {
            mInstance.destroyDebugUtilsMessengerEXT(mDebugMessenger);
        }
        if(!!mDebugReport)
        {
            mInstance.destroyDebugReportCallbackEXT(mDebugReport);
        }

        mInstance.destroy();
    }

    VkBool32 VulkanInstance::OnDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                      VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData)
    {
        switch(messageSeverity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                logger()->info("{}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                logger()->info("{}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                logger()->warn("{}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                logger()->error("{}", pCallbackData->pMessage);
                if(mThrowOnValidationError)
                {
                    throw Exception("{}", pCallbackData->pMessage);
                }
                break;
            default:
                break;
        }
        return VK_FALSE;
    }

    std::unordered_map<VkDebugReportFlagsEXT, spdlog::level::level_enum> levelMap = {
        {VK_DEBUG_REPORT_INFORMATION_BIT_EXT, spdlog::level::info},
        {VK_DEBUG_REPORT_WARNING_BIT_EXT, spdlog::level::warn},
        {VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, spdlog::level::warn},
        {VK_DEBUG_REPORT_ERROR_BIT_EXT, spdlog::level::err},
        {VK_DEBUG_REPORT_DEBUG_BIT_EXT, spdlog::level::info},
    };


    VkBool32 VulkanInstance::OnDebugReportCallback(
        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage)
    {
        spdlog::level::level_enum severity = levelMap.at(flags);
        logger()->log(severity, "DebugReport[{}] {}", pLayerPrefix, pMessage);
        return VK_FALSE;
    }

    void VulkanInstance::CreateInfo::SetDefaults(View<osi::Window> window)
    {
        // Application Info
        mAppInfo.pEngineName   = "Foray";
        mAppInfo.engineVersion = vk::makeApiVersion(0, 2, 0, 0);
        // Validation Layers
        mEnabledLayers.Add(LayerNames::Validation);
        // Debug Printf
        mValidationFeatureEnables.emplace(vk::ValidationFeatureEnableEXT::eDebugPrintf);
        mEnabledExtensions.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        mEnabledExtensions.Add(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        mDesiredApiVersion  = vk::ApiVersion13;
        mRequiredApiVersion = vk::ApiVersion12;
        // Surface Extensions
        if(!!window)
        {
            std::vector<const char*> extensions = window->GetVkSurfaceExtensions();
            mEnabledExtensions.AddRange(extensions.begin(), extensions.end());
        }
    }

    std::span<const VulkanAdapterInfo> VulkanInstance::GetAdapterInfos() const
    {
        return mAdapters;
    }

}  // namespace foray::base
