#include "foray_vkb.hpp"

namespace foray {
    void SetVulkanObjectName(const vkb::DispatchTable* dispatchTable, VkObjectType objectType, const void* objectHandle, std::string_view name)
    {
        std::string                   namecopy(name);
        VkDebugUtilsObjectNameInfoEXT nameInfo{.sType        = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                                               .pNext        = nullptr,
                                               .objectType   = objectType,
                                               .objectHandle = reinterpret_cast<uint64_t>(objectHandle),
                                               .pObjectName  = namecopy.data()};
        dispatchTable->setDebugUtilsObjectNameEXT(&nameInfo);
    }
}  // namespace foray
