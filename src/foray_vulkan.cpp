#include "foray_vulkan.hpp"
#include <nameof/nameof.hpp>
#include "core/foray_vkcontext.hpp"

namespace foray {
    std::string_view PrintVkResult(VkResult result)
    {
        return NAMEOF_ENUM(result);
    }

    void SetVulkanObjectName(const core::VkContext* context, VkObjectType objectType, const void* objectHandle, std::string_view name)
    {
        SetVulkanObjectName(&(context->DispatchTable), objectType, objectHandle, name);
    }

}  // namespace foray
