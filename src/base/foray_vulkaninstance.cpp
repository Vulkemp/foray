#include "foray_vulkaninstance.hpp"
#include "../foray_exception.hpp"
#include "../foray_logger.hpp"
#include "../foray_vulkan.hpp"
#include "../core/foray_context.hpp"
#include "../osi/foray_window.hpp"

namespace foray::base {
    VulkanInstance& VulkanInstance::SetBeforeInstanceBuildFunc(BeforeInstanceBuildFunctionPointer beforeInstanceBuildFunc)
    {
        mBeforeInstanceBuildFunc = beforeInstanceBuildFunc;
        return *this;
    }
    VulkanInstance& VulkanInstance::SetDebugCallbackFunc(PFN_vkDebugUtilsMessengerCallbackEXT debugCallbackFunc)
    {
        mDebugCallbackFunc = debugCallbackFunc;
        return *this;
    }


    void VulkanInstance::Create()
    {
        vkb::InstanceBuilder instanceBuilder;

        if(mEnableDebugLayersAndCallbacks)
        {
            instanceBuilder.enable_validation_layers();
            if(!!mDebugCallbackFunc)
            {
                instanceBuilder.set_debug_callback(mDebugCallbackFunc);
            }
            if(!!mDebugUserData)
            {
                instanceBuilder.set_debug_callback_user_data_pointer(mDebugUserData);
            }
        }
        if (!!mContext && !!mContext->Window)
        {
            std::vector<const char*> surfaceExtensions = mContext->Window->GetVkSurfaceExtensions();
            for (const char* ext : surfaceExtensions)
            {
                instanceBuilder.enable_extension(ext);
            }
        }
        instanceBuilder.set_minimum_instance_version(VK_VERSION_1_3);
        if(!!mBeforeInstanceBuildFunc)
        {
            mBeforeInstanceBuildFunc(instanceBuilder);
        }
        auto ret = instanceBuilder.build();
        FORAY_ASSERTFMT(ret.has_value(), "[VulkanInstance::Create] vkb Instance Builder failed to build instance. VkResult: {} Reason: {}", PrintVkResult(ret.vk_result()),
                        ret.error().message())

        mInstance = *ret;
        if (!!mContext)
        {
            mContext->VkbInstance = &mInstance;
        }
    }

    void VulkanInstance::Destroy()
    {
        if(!!mInstance.instance)
        {
            vkb::destroy_instance(mInstance);
            mInstance = vkb::Instance();
        }
        if (!!mContext)
        {
            mContext->VkbInstance = nullptr;
        }
    }

    VulkanInstance::~VulkanInstance()
    {
        if(!!mInstance.instance)
        {
            vkb::destroy_instance(mInstance);
            mInstance = vkb::Instance();
        }
        if (!!mContext)
        {
            mContext->VkbInstance = nullptr;
        }
    }

    VkBool32 VulkanInstance::DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                  VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                  void*                                       pUserData)
    {
        auto severity = vkb::to_string_message_severity(messageSeverity);
        auto type     = vkb::to_string_message_type(messageTypes);
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
                throw Exception("{}", pCallbackData->pMessage);
                break;
            default:
                break;
        }
        return VK_FALSE;
    }

}  // namespace foray::base
