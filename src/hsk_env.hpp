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

    std::string MakeRelativePath(const std::string_view relative);

    /// @brief Utf8 encoded path wrapper. Unlike std::filesystem::path uses the same encoding regardless of compile target and path / additions automatically resolves navigator
    class Utf8Path
    {
      protected:
        std::string mPath;
        bool mRelative;

        void VerifyPath();
      public:
        Utf8Path();
        Utf8Path(std::string_view path);
        Utf8Path(std::u8string_view path);
        Utf8Path(std::filesystem::path path);

        Utf8Path  operator/(const Utf8Path& other) const;
        Utf8Path& operator/=(const Utf8Path& other);

        operator const std::string&() const;
        operator const char*() const;
        operator std::filesystem::path() const;

        bool     IsRelative() const;
        Utf8Path MakeAbsolute() const;
    };
}  // namespace hsk