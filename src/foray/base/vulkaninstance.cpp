#include "vulkaninstance.hpp"
#include "../core/context.hpp"
#include "../exception.hpp"
#include "../logger.hpp"
#include "../vulkan.hpp"
#include "../osi/window.hpp"

namespace foray::base {

    void VulkanInstance::Create()
    {
        vkb::InstanceBuilder instanceBuilder;

        if(mEnableDebugLayersAndCallbacks)
        {
            instanceBuilder.enable_validation_layers();
            if(!!mDebugMessengerFunc)
            {
                instanceBuilder.set_debug_callback(mDebugMessengerFunc);
            }
            if(mDebugMessengerFunc == &DefaultDebugMessengerCallback)
            {
                instanceBuilder.set_debug_callback_user_data_pointer(this);
            }
            else if(!!mDebugUserData)
            {
                instanceBuilder.set_debug_callback_user_data_pointer(mDebugUserData);
            }
        }
        if(mEnableDebugReport)
        {
            instanceBuilder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
            instanceBuilder.enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            instanceBuilder.enable_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }
        instanceBuilder.require_api_version(VK_MAKE_API_VERSION(0, 1, 3, 0));
        instanceBuilder.set_minimum_instance_version(VK_MAKE_API_VERSION(0, 1, 3, 0));
        if(!!mContext && !!mContext->WindowSwapchain)
        {
            std::vector<const char*> surfaceExtensions = mContext->WindowSwapchain->GetWindow().GetVkSurfaceExtensions();
            for(const char* ext : surfaceExtensions)
            {
                instanceBuilder.enable_extension(ext);
            }
        }
        if(!!mBeforeInstanceBuildFunc)
        {
            mBeforeInstanceBuildFunc(instanceBuilder);
        }
        auto ret = instanceBuilder.build();
        FORAY_ASSERTFMT(ret.has_value(), "[VulkanInstance::Create] vkb Instance Builder failed to build instance. VkResult: {} Reason: {}", PrintVkResult(ret.vk_result()),
                        ret.error().message())

        mInstance = *ret;
        if(!!mContext)
        {
            mContext->Instance = this;
        }
        if(mEnableDebugReport)
        {
            // Populate the VkDebugReportCallbackCreateInfoEXT
            VkDebugReportCallbackCreateInfoEXT ci = {};
            ci.sType                              = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            ci.pfnCallback                        = mDebugReportFunc;
            ci.flags                              = VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
            ci.pUserData                          = nullptr;

            PFN_vkCreateDebugReportCallbackEXT createDebugReportCallback = VK_NULL_HANDLE;
            createDebugReportCallback                                    = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugReportCallbackEXT");

            // Create the callback handle
            createDebugReportCallback(mContext->VkInstance(), &ci, nullptr, &mDebugReportCallbackHandle);
        }
    }

    VulkanInstance::~VulkanInstance()
    {
        if(!!mDebugReportCallbackHandle)
        {
            PFN_vkDestroyDebugReportCallbackEXT destroyDebugReportCallback = VK_NULL_HANDLE;
            destroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugReportCallbackEXT");

            // Create the callback handle
            destroyDebugReportCallback(mContext->VkInstance(), mDebugReportCallbackHandle, nullptr);
            mDebugReportCallbackHandle = nullptr;
        }
        if(!!mInstance.instance)
        {
            vkb::destroy_instance(mInstance);
            mInstance = vkb::Instance();
        }
        if(!!mContext)
        {
            mContext->Instance = nullptr;
        }
    }

    VkBool32 VulkanInstance::DefaultDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                           VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                           void*                                       pUserData)
    {
        bool throwOnError = true;
        const VulkanInstance* vkinst = (const VulkanInstance*)pUserData;
        if (!!vkinst)
        {
            throwOnError = vkinst->GetThrowOnValidationError();
        }
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
                if(throwOnError)
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


    VkBool32 VulkanInstance::DefaultDebugReportCallback(VkDebugReportFlagsEXT      flags,
                                                        VkDebugReportObjectTypeEXT objectType,
                                                        uint64_t                   object,
                                                        size_t                     location,
                                                        int32_t                    messageCode,
                                                        const char*                pLayerPrefix,
                                                        const char*                pMessage,
                                                        void*                      pUserData)
    {
        spdlog::level::level_enum severity = levelMap.at(flags);
        logger()->log(severity, "DebugReport[{}] {}", pLayerPrefix, pMessage);
        return VK_FALSE;
    }

}  // namespace foray::base
