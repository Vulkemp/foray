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
#undef CreateWindow //
#endif

namespace foray {
    /// @brief Prints a VkResult. If outside of NAMEF_ENUM_RANGE_MAX, will only print number
    std::string PrintVkResult(VkResult result);

    /// @brief Asserts a VkResult (Fails if not VK_SUCCESS)
    inline void AssertVkResult(VkResult result, const source_location location = source_location::current())
    {
        if(result != VK_SUCCESS)
        {
            Exception::Throw(location, "VkResult Assertion Failed: VkResult::{}", PrintVkResult(result));
        }
    }

    /// @brief Set a vulkan object name (Will show up in validation errors, NSight, RenderDoc, ...)
    /// @param context Requires DispatchTable
    /// @param objectType Object Type passed as objectHandle
    /// @param objectHandle Vulkan Handle
    /// @param name Debug name
    void SetVulkanObjectName(core::Context* context, VkObjectType objectType, const void* objectHandle, std::string_view name);

}  // namespace foray
