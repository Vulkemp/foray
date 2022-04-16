#pragma once
#include <filesystem>
#include <string_view>

namespace hsk{
    std::string_view CurrentWorkingDirectory();
    void UpdateCurrentWorkingDirectory();

    std::string MakeRelativePath(std::string_view relative);
}