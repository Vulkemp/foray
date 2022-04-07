#include "importance_sampling_rt.hpp"
#include <nameof/nameof.hpp>

// std libs
#include <iostream>

// rtrpf
#include <spdlog/spdlog.h>

int main(int argv, char** args)
{
    ImportanceSamplingRtProject project;
    return project.Run();
}

void ImportanceSamplingRtProject::Init()
{
    mWindow.DisplayMode(hsk::EDisplayMode::WindowedResizable);
    initVulkan();
}

void ImportanceSamplingRtProject::OnEvent(hsk::Event::ptr event)
{
    auto buttonInput = std::dynamic_pointer_cast<hsk::EventInputBinary>(event);
    auto axisInput   = std::dynamic_pointer_cast<hsk::EventInputAnalogue>(event);
    if(buttonInput)
    {
        spdlog::info("Device \"{}\" Button {} - {}", buttonInput->Device()->Name(), buttonInput->Button()->Name(), buttonInput->Pressed() ? "pressed" : "released");
    }
    if(axisInput)
    {
        spdlog::info("Device \"{}\" Axis {} - {}", axisInput->Device()->Name(), axisInput->Axis()->Name(), axisInput->State());
    }
}
