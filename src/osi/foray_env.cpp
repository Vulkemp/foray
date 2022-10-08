#include "foray_env.hpp"

#include "../core/foray_logger.hpp"
#include "../foray_exception.hpp"

#ifdef WIN32
#include <stringapiset.h>
#endif

namespace foray::osi {

#ifdef WIN32
    std::filesystem::path FromUtf8Path(std::string_view utf8path)
    {
        int          wstr_size = MultiByteToWideChar(CP_UTF8, 0, utf8path.data(), (int)utf8path.size(), nullptr, 0);
        std::wstring wstr(wstr_size, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8path.data(), (int)utf8path.size(), &wstr[0], (int)wstr.size());
        return std::filesystem::path(wstr);
    }
    std::filesystem::path FromUtf8Path(std::u8string_view utf8path)
    {
        std::string_view cstr(reinterpret_cast<const char*>(utf8path.data()), utf8path.size());
        int              wstr_size = MultiByteToWideChar(CP_UTF8, 0, cstr.data(), (int)cstr.size(), nullptr, 0);
        std::wstring     wstr(wstr_size, 0);
        MultiByteToWideChar(CP_UTF8, 0, cstr.data(), (int)cstr.size(), &wstr[0], (int)wstr.size());
        return std::filesystem::path(wstr);
    }
    std::string ToUtf8Path(std::filesystem::path path)
    {
        std::wstring_view wstr     = path.c_str();
        int               str_size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, NULL, NULL);
        std::string       str(str_size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &str[0], (int)str.size(), NULL, NULL);
        return str;
    }
#else
    std::filesystem::path FromUtf8Path(std::string_view utf8path)
    {
        std::u8string_view u8str(reinterpret_cast<const char8_t*>(utf8path.data()), utf8path.size());
        return std::filesystem::path(u8str);
    }
    std::filesystem::path FromUtf8Path(std::u8string_view utf8path)
    {
        return std::filesystem::path(utf8path);
    }
    std::string ToUtf8Path(std::filesystem::path path)
    {
        return std::string(reinterpret_cast<const char*>(path.c_str()));
    }
#endif


    std::filesystem::path cwd     = std::filesystem::path();
    std::string           cwd_str = "";

    void UpdateCurrentWorkingDirectory()
    {
        cwd     = std::filesystem::current_path();
        cwd_str = ToUtf8Path(cwd);
    }

    std::string_view CurrentWorkingDirectory()
    {
        if(cwd.empty())
        {
            UpdateCurrentWorkingDirectory();
        }
        return cwd_str;
    }

    void OverrideCurrentWorkingDirectory(std::string_view path)
    {
        cwd = FromUtf8Path(path);

        core::logger()->info("Setting working directory to \"{}\"", path);

        std::filesystem::current_path(cwd);
        cwd_str = path;
    }

    std::string MakeRelativePath(const std::string_view relative)
    {
        if(cwd.empty())
        {
            UpdateCurrentWorkingDirectory();
        }

        std::filesystem::path path = cwd;
        path /= std::filesystem::path(FromUtf8Path(relative));
        return ToUtf8Path(path);
    }

    Utf8Path::Utf8Path() : mPath(), mRelative(true), mPathSections() {}

    Utf8Path::Utf8Path(const std::vector<std::string_view>& sections, bool relative) : mPath(), mRelative(relative), mPathSections(sections)
    {
        sBuildFromSections(mPath, mPathSections);
        BuildSectionVector();
    }

    void Utf8Path::VerifyPath()
    {
        Assert(!mPath.empty(), "Invalid Path Initializer: path argument may not be empty!");
#ifdef WIN32
        mRelative = !(mPath.size() >= 2 && mPath[1] == ':' && (mPath[2] == '\\' || mPath[2] == '/'));
        // If the path came from std::filesystem::path, it will contain \ characters
        for(char& c : mPath)
        {
            if(c == '\\')
            {
                c = '/';
            }
        }
#else
        mRelative = !(mPath.starts_with("/") || mPath.starts_with("~/"));
#endif

        // Collapse navigator paths

        BuildSectionVector();

        bool attemptCollapse = false;
        for(int32_t i = 1; i < mPathSections.size(); i++)
        {
            std::string_view section = mPathSections[i];
            if(section == "." || section == "..")
            {
                attemptCollapse = true;
                break;
            }
        }

        if(attemptCollapse)
        {
            std::vector<std::string_view> path;

            for(std::string_view section : mPathSections)
            {
                if(section == "..")
                {
                    if(path.empty() || path.back() == "..")
                    {
                        Assert(mRelative, "Invalid path operation: Navigating above root directory!");
                        path.push_back(section);
                    }
                    else
                    {
                        path.pop_back();
                    }
                }
                else if(section != ".")
                {
                    path.push_back(section);
                }
            }

            mPathSections = path;

            sBuildFromSections(mPath, mPathSections);
            BuildSectionVector();
        }
    }

    void Utf8Path::BuildSectionVector()
    {
        mPathSections.clear();

        int32_t pos   = 0;
        int32_t start = 0;
        for(; pos < mPath.size(); pos++)
        {
            char c = mPath[pos];
            if(c == '/')
            {
                std::string_view substr(mPath.data() + start, pos - start);
                mPathSections.push_back(substr);
                start = pos + 1;
            }
        }
        std::string_view substr(mPath.data() + start, pos - start);
        mPathSections.push_back(substr);
    }
    void Utf8Path::sBuildFromSections(std::string& path, const std::vector<std::string_view>& sections)
    {

        std::stringstream strbuilder;
        for(int i = 0; i < sections.size() - 1; i++)
        {
            strbuilder << sections[i] << "/";
        }
        if(!sections.empty())
        {
            strbuilder << sections.back();
        }
        path = strbuilder.str();
    }

    Utf8Path Utf8Path::operator/(const Utf8Path& other) const
    {
        Assert(other.IsRelative(), "Invalid path operation: Cannot resolve absolute path as relative!");
        const std::vector<std::string_view> right = other.mPathSections;

        std::vector<std::string_view> path = mPathSections;
        path.reserve(mPathSections.size() + right.size());

        // Resolve ../ and ./ directory actions

        for(std::string_view section : right)
        {
            if(section == "..")
            {
                Assert(mRelative || !path.empty(), "Invalid path operation: Navigating above root directory!");
                if(!path.empty() && path.back() == "..")
                {
                    path.push_back(section);
                }
                else
                {
                    path.pop_back();
                }
            }
            else if(section != ".")
            {
                path.push_back(section);
            }
        }

        return Utf8Path(path, mRelative);
    }
    Utf8Path& Utf8Path::operator/=(const Utf8Path& other)
    {
        *this = *this / other;
        return *this;
    }

    bool Utf8Path::operator==(const Utf8Path& other) const
    {
        return mRelative == other.mRelative && other.mPath == other.mPath;
    }
    bool Utf8Path::operator!=(const Utf8Path& other) const
    {
        return mRelative != other.mRelative || other.mPath != other.mPath;
    }
    bool Utf8Path::operator<(const Utf8Path& other) const
    {
        Assert(!mRelative && !other.mRelative, "Lexical comparison is only valid for absolute paths!");
        return mPath < other.mPath;
    }
    bool Utf8Path::operator>(const Utf8Path& other) const
    {
        Assert(!mRelative && !other.mRelative, "Lexical comparison is only valid for absolute paths!");
        return mPath > other.mPath;
    }

    Utf8Path::operator const std::string&() const
    {
        return mPath;
    }
    Utf8Path::operator const char*() const
    {
        return mPath.data();
    }
    Utf8Path::operator std::filesystem::path() const
    {
        return FromUtf8Path(mPath);
    }

    bool Utf8Path::IsRelative() const
    {
        return mRelative;
    }
    Utf8Path Utf8Path::MakeAbsolute() const
    {
        return Utf8Path(CurrentWorkingDirectory()) / *this;
    }

}  // namespace foray::osi
