#pragma once
#include "../foray_basics.hpp"
#include "../foray_logger.hpp"
#include <filesystem>
#include <vector>

namespace foray::osi {
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
        /// @brief The stored path, encoded as UTF-8
        std::string mPath;
        /// @brief If true, the path is a relative path
        bool mRelative;
        /// @brief Directory and file names making up this path. References sections in mPath, result of splitting by '/'. May contain empty entries (for example absolute paths on unix systems)
        std::vector<std::string_view> mPathSections;

        /// @brief Detects absolute paths, and clears '../' and './' navigators where possible
        void VerifyPath();
        /// @brief Updates mPathSections
        void BuildSectionVector();
        /// @brief Builds a new path string (stored to path parameter) from sections
        static void sBuildFromSections(std::string& path, const std::vector<std::string_view>& sections);

        Utf8Path(const std::vector<std::string_view>& sections, bool relative);

      public:
        inline Utf8Path(const Utf8Path& other) : mPath(other.mPath) { VerifyPath(); }
        inline Utf8Path(Utf8Path&& other) : mPath(other.mPath) { VerifyPath(); }
        Utf8Path& operator=(const Utf8Path& other);
        /// @brief Default constructs with empty relative path
        Utf8Path();
        template <typename StringViewLike>
        Utf8Path(const StringViewLike& path) : mPath(path), mRelative()
        {
            VerifyPath();
        }

        /// @brief Combine paths, interpreted as navigating relative of this path to the other path
        Utf8Path operator/(const Utf8Path& other) const;
        /// @brief Update this paths by navigating relative to self as dictated by other
        Utf8Path& operator/=(const Utf8Path& other);

        /// @brief Gets the internally stored path
        operator const std::string&() const;
        /// @brief Gets the internally stored path
        operator const char*() const;
        /// @brief Gets the internally stored path
        operator std::filesystem::path() const;

        bool operator==(const Utf8Path& other) const;
        bool operator!=(const Utf8Path& other) const;
        bool operator<(const Utf8Path& other) const;
        bool operator>(const Utf8Path& other) const;

        bool     IsRelative() const;
        Utf8Path MakeAbsolute() const;

        FORAY_GETTER_V(Path)
        FORAY_GETTER_CR(PathSections)
    };
}  // namespace foray::osi

namespace std {

    template <>
    struct hash<foray::osi::Utf8Path>
    {
        inline size_t operator()(const foray::osi::Utf8Path& p) const {
            foray::logger()->info("Path \"{}\": 0x{:x}", p, std::hash<std::string>().operator()(p.GetPath()));

            return std::hash<std::string>()(p.GetPath()); 
        }
    };
}  // namespace std
