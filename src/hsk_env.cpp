#include "hsk_env.hpp"

namespace hsk {

    std::filesystem::path cwd = std::filesystem::path();

    std::string_view CurrentWorkingDirectory()
    {
        if(cwd.empty())
        {
            UpdateCurrentWorkingDirectory();
        }
        return cwd.c_str();
    }

    void UpdateCurrentWorkingDirectory() { cwd = std::filesystem::current_path(); }

    std::string MakeRelativePath(std::string_view relative) {
        if (cwd.empty()){
            UpdateCurrentWorkingDirectory();
        }
        
        std::string cwdstr = cwd.c_str();
        std::filesystem::path path = cwd;
        path /= std::filesystem::path(relative);
        cwdstr = path.c_str();
        return path.c_str();
    }
}  // namespace hsk
