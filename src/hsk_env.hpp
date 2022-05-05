#pragma once
#include <filesystem>
#include <string_view>

namespace hsk{


    std::string CurrentWorkingDirectory();
    void UpdateCurrentWorkingDirectory();

    void OverrideCurrentWorkingDirectory(std::string_view path);

    std::string MakeRelativePath(std::string_view relative);
}