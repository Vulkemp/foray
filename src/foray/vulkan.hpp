#pragma once
#include "core/core_declares.hpp"
#include "basics.hpp"
#include "exception.hpp"
#include <vulkan/vulkan.hpp>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#undef max // Windows headers included hide std::max
#undef min // Windows headers included hide std::max
#undef CreateWindow //
#endif

namespace foray {
    /// @brief Prints a vk::Result. If outside of NAMEF_ENUM_RANGE_MAX, will only print number
    std::string PrintVkResult(vk::Result result);

    /// @brief Asserts a vk::Result (Fails if not VK_SUCCESS)
    inline void AssertVkResult(vk::Result result, const source_location location = source_location::current())
    {
        if(result != vk::Result::eSuccess)
        {
            Exception::Throw(location, "vk::Result Assertion Failed: vk::Result::{}", PrintVkResult(result));
        }
    }

    /// @brief Set a vulkan object name (Will show up in validation errors, NSight, RenderDoc, ...)
    /// @param context Requires DispatchTable
    /// @param objectType Object Type passed as objectHandle
    /// @param objectHandle Vulkan Handle
    /// @param name Debug name
    void SetVulkanObjectName(core::Context* context, vk::ObjectType objectType, const void* objectHandle, std::string_view name);

}  // namespace foray
