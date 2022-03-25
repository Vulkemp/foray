#pragma once

#include <hsk_rtrpf.hpp>
#include <stdint.h>

class ImportanceSamplingRtProject : public hsk::MinimalAppBase
{
public:
	ImportanceSamplingRtProject() = default;
	~ImportanceSamplingRtProject() = default;

protected:

	virtual void Init() override;
};