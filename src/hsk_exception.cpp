#include "hsk_exception.hpp"

namespace hsk {
    void Exception::Throw(std::string_view reason, const std::source_location location)
    {
        std::string reasonWithLocation = fmt::format("{} (\"{}\":{}:{}): {}", location.function_name(), location.file_name(), location.line(), location.column(), reason);
        Throw(reasonWithLocation);
    }
}  // namespace hsk