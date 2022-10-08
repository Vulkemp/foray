#include "foray_exception.hpp"

namespace foray {
    void Exception::Throw(std::string_view reason, const source_location location)
    {
        std::string reasonWithLocation = fmt::format("{} (\"{}\":{}:{}): {}", location.function_name(), location.file_name(), location.line(), location.column(), reason);
        throw Exception(reasonWithLocation);
    }
}  // namespace foray