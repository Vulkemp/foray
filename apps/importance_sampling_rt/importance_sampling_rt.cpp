#include "importance_sampling_rt.hpp"

// std libs
#include <iostream>

// rtrpf
#include "hsk_base_app.hpp"
#include <spdlog/spdlog.h>

int main()
{
	ImportanceSamplingRtProject project;
	return project.Run();
}

int32_t ImportanceSamplingRtProject::Run()
{
	spdlog::info("hello");
	return 0;
}

