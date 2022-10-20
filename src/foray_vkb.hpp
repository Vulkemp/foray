#pragma once
#include "foray_vulkan.hpp"
#include <vkbootstrap/VkBootstrap.h>

namespace foray {
    void SetVulkanObjectName(const vkb::DispatchTable* dispatchTable, VkObjectType objectType, const void* handle, std::string_view name);
}