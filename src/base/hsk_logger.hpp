#pragma once
#include <spdlog/spdlog.h>

namespace hsk {
    /// @brief Gives access to a global available logger object.
    /// The logger writes to console & to a file next to the application in the folder /logs.
    spdlog::logger* logger();
}  // namespace hsk
