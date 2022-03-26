#include "importance_sampling_rt.hpp"

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
}

