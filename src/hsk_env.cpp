#include "hsk_env.hpp"
#include "base/hsk_logger.hpp"
#ifdef WIN32
#include <stringapiset.h>
#endif

namespace hsk {

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

        logger()->info("Setting working directory to \"{}\"", path);

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

    Utf8Path::Utf8Path() : mPath() {}
    Utf8Path::Utf8Path(std::string_view path) : mPath(path)
    {
        VerifyPath();
    }
    Utf8Path::Utf8Path(std::u8string_view path) : mPath(reinterpret_cast<const char*>(path.data()), path.size())
    {
        VerifyPath();
    }
    Utf8Path::Utf8Path(std::filesystem::path path) : mPath(ToUtf8Path(path))
    {
        VerifyPath();
    }

    void Utf8Path::VerifyPath()
    {
        Assert(!mPath.empty(), "Invalid Path Initializer: path argument may not be empty!");
#ifdef WIN32
        mRelative = !(mPath.size() >= 2 && mPath[1] == ":" && mPath[2] == "\\");
        // If the path came from std::filesystem::path, it will contain \ characters
        for(char& c : mPath)
        {
            if(c == '\\')
            {
                c = '/';
            }
        }
#else
        mRelative = !mPath.starts_with("/");
#endif
    }

    Utf8Path Utf8Path::operator/(const Utf8Path& other) const
    {
        Assert(other.IsRelative(), "Invalid path operation: Cannot resolve absolute path as relative!");
        std::string       right = other;
        std::vector<char> path;
        path.reserve(mPath.size() + right.size() + 1);
        for(char c : mPath)
        {
            path.push_back(c);
        }

        // Resolve ../ and ./ directory actions
        while(true)
        {
            if(right.starts_with("../"))
            {
                right = right.substr(3);
                while(true)
                {
                    Assert(!path.empty(), "Invalid path operation: Navigating above root directory!");
                    if(path.back() == '/')
                    {
                        path.pop_back();
                        break;
                    }
                    path.pop_back();
                }
                continue;
            }
            if(right.starts_with("./"))
            {
                right = right.substr(2);
                continue;
            }
            break;
        }

        path.push_back('/');
        for(char c : right)
        {
            path.push_back(c);
        }
        std::string_view pathstr(path.data(), path.size());
        return Utf8Path(pathstr);
    }
    Utf8Path& Utf8Path::operator/=(const Utf8Path& other)
    {
        *this = *this / other;
        return *this;
    }
    Utf8Path::operator const std::string&() const
    {
        return mPath;
    }
    Utf8Path::operator const char*() const
    {
        return mPath.data();
    }
    bool Utf8Path::IsRelative() const
    {
        return mRelative;
    }
    Utf8Path Utf8Path::MakeAbsolute() const
    {
        return Utf8Path(CurrentWorkingDirectory()) / *this;
    }

}  // namespace hsk
