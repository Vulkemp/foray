#include "foray_vulkan.hpp"
#include <nameof/nameof.hpp>
#include "core/foray_vkcontext.hpp"

namespace foray {
    std::string_view PrintVkResult(VkResult result)
    {
        return NAMEOF_ENUM(result);
    }

    void SetVulkanObjectName(const core::VkContext* context, VkObjectType objectType, const void* objectHandle, const std::string_view& name)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{.sType        = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                                               .pNext        = nullptr,
                                               .objectType   = objectType,
                                               .objectHandle = reinterpret_cast<uint64_t>(objectHandle),
                                               .pObjectName  = name.data()};
        context->DispatchTable.setDebugUtilsObjectNameEXT(&nameInfo);
    }

}  // namespace foray
