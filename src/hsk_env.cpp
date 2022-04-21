#include "hsk_env.hpp"

namespace hsk {

// Windows does not support unicode encoded filepaths in UTF8, therefor we need to translate (As all third party libraries use UTF8, we work on paths through UTF8)

#ifdef WIN32
    using str_t     = std::wstring;
    using strview_t = std::wstring_view;

    // pulled from tinygltf
    static inline std::wstring UTF8ToWchar(const std::string_view& str)
    {
        int          wstr_size = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
        std::wstring wstr(wstr_size, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], (int)wstr.size());
        return wstr;
    }

    // pulled from tinygltf
    static inline std::string WcharToUTF8(const std::wstring_view& wstr)
    {
        int         str_size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, NULL, NULL);
        std::string str(str_size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &str[0], (int)str.size(), NULL, NULL);
        return str;
    }

#else
    using str_t     = std::string;
    using strview_t = std::string_view;
#endif
    std::filesystem::path cwd = std::filesystem::path();

    std::string CurrentWorkingDirectory()
    {
        if(cwd.empty())
        {
            UpdateCurrentWorkingDirectory();
        }
#ifdef WIN32
        std::wstring cstr = cwd.c_str();
        return WcharToUTF8(cstr);
#else
        return cwd.c_str();
#endif
    }

    void UpdateCurrentWorkingDirectory() { cwd = std::filesystem::current_path(); }

    std::string MakeRelativePath(std::string_view relative)
    {
        if(cwd.empty())
        {
            UpdateCurrentWorkingDirectory();
        }

#ifdef WIN32
        std::filesystem::path path   = cwd;
        std::wstring wstrrelative = UTF8ToWchar(relative);
        path /= std::filesystem::path(relative);
        std::wstring cstr = path.c_str();
        return WcharToUTF8(cstr);
#else
        std::filesystem::path path   = cwd;
        path /= std::filesystem::path(relative);
        return path.c_str();
#endif
    }
}  // namespace hsk
