#include "hsk_vkHelpers.hpp"
#include <nameof/nameof.hpp>

namespace hsk {
    std::string_view PrintVkResult(VkResult result) { return NAMEOF_ENUM(result); }
}  // namespace hsk