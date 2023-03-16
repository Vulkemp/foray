#include "foray_vulkan.hpp"
#include <nameof/nameof.hpp>
#include "core/foray_context.hpp"

namespace foray {
    std::string PrintVkResult(VkResult result)
    {
        if (result > NAMEOF_ENUM_RANGE_MAX)
        {
            return fmt::format("VkResult.{}", (int32_t)result);
        }
        return std::string(NAMEOF_ENUM(result));
    }

    void SetVulkanObjectName(core::Context* context, VkObjectType objectType, const void* objectHandle, std::string_view name)
    {
        std::string                   namecopy(name);
        VkDebugUtilsObjectNameInfoEXT nameInfo{.sType        = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                                               .pNext        = nullptr,
                                               .objectType   = objectType,
                                               .objectHandle = reinterpret_cast<uint64_t>(objectHandle),
                                               .pObjectName  = namecopy.data()};
        context->VkbDispatchTable->setDebugUtilsObjectNameEXT(&nameInfo);
    }

}  // namespace foray
