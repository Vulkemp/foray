#pragma once
#include "core/foray_core_declares.hpp"
#include "foray_basics.hpp"
#include "foray_exception.hpp"
#include <vulkan/vulkan.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#undef max // Windows headers included hide std::max
#undef min // Windows headers included hide std::max
#endif

namespace foray {
    std::string_view PrintVkResult(VkResult result);

    inline void AssertVkResult(VkResult result, const source_location location = source_location::current())
    {
        if(result != VK_SUCCESS)
        {
            Exception::Throw(location, "VkResult Assertion Failed: VkResult::{}", PrintVkResult(result));
        }
    }

    void SetVulkanObjectName(core::Context* context, VkObjectType objectType, const void* objectHandle, std::string_view name);

}  // namespace foray
