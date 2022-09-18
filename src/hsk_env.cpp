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
}  // namespace hsk
