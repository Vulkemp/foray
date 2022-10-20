#include "foray_vulkaninstance.hpp"
#include "../foray_exception.hpp"
#include "../foray_logger.hpp"
#include "../foray_vulkan.hpp"

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
        if(!!mBeforeInstanceBuildFunc)
        {
            mBeforeInstanceBuildFunc(instanceBuilder);
        }
        auto ret = instanceBuilder.build();
        FORAY_ASSERTFMT(ret.has_value(), "[VulkanInstance::Create] vkb Instance Builder failed to build instance. VkResult: {} Reason: {}", PrintVkResult(ret.vk_result()),
                        ret.error().message())

        mInstance = *ret;
    }

    void VulkanInstance::Destroy()
    {
        if(!!mInstance.instance)
        {
            vkb::destroy_instance(mInstance);
            mInstance = vkb::Instance();
        }
    }

    VulkanInstance::~VulkanInstance()
    {
        if(!!mInstance.instance)
        {
            vkb::destroy_instance(mInstance);
            mInstance = vkb::Instance();
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
