#pragma once

#include <hsk_base_app.hpp>
#include <stdint.h>

class ImportanceSamplingRtProject : public hsk::BaseApp
{
public:
	ImportanceSamplingRtProject() = default;
	~ImportanceSamplingRtProject() = default;

	int32_t Run();

protected:
};