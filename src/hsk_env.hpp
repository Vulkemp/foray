#pragma once
#include <filesystem>
#include <string_view>

namespace hsk {
    /// @brief Convert UTF8 string path to absolute std::filesystem::path type
    std::filesystem::path FromUtf8Path(std::string_view utf8path);
    /// @brief Convert UTF8 string path to absolute std::filesystem::path type
    std::filesystem::path FromUtf8Path(std::u8string_view utf8path);
    /// @brief Convert std::filesystem::path type to UTF8 path
    std::string ToUtf8Path(std::filesystem::path path);

    std::string_view CurrentWorkingDirectory();

    void OverrideCurrentWorkingDirectory(std::string_view path);

    std::string MakeRelativePath(std::string_view relative);
}  // namespace hsk