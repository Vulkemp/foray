#pragma once

#include <hsk_rtrpf.hpp>
#include <stdint.h>

class ImportanceSamplingRtProject : public hsk::DefaultAppBase
{
public:
	ImportanceSamplingRtProject() = default;
	~ImportanceSamplingRtProject() = default;

protected:

	virtual void Init() override;
	virtual void OnEvent(hsk::Event::ptr event) override;
};