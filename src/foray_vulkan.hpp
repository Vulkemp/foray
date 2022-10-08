#pragma once
#include "core/foray_core_declares.hpp"
#include "foray_basics.hpp"
#include "foray_exception.hpp"
#include <vulkan/vulkan.h>

namespace foray {
    std::string_view PrintVkResult(VkResult result);

    inline void AssertVkResult(VkResult result, const source_location location = source_location::current())
    {
        if(result != VK_SUCCESS)
        {
            Exception::Throw(location, "VkResult Assertion Failed: VkResult::{}", PrintVkResult(result));
        }
    }

    void SetVulkanObjectName(const core::VkContext* context, VkObjectType objectType, const void* objectHandle, const std::string_view& name);

}  // namespace foray
