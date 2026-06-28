#pragma once

#include "Utils/myn/Timer.h"

class FpsMeter
{
public:
	void beginFrame();
	void endFrame(float elapsed);
	void draw() const;

private:
	static constexpr float update_interval = 0.5f;

	myn::TimePoint cpu_frame_start;
	float accumulated_time = 0.0f;
	float accumulated_cpu_frame_time_ms = 0.0f;
	float accumulated_gpu_frame_time_ms = 0.0f;
	int accumulated_frames = 0;
	int accumulated_gpu_frames = 0;
	float fps = 0.0f;
	float cpu_frame_time_ms = 0.0f;
	float gpu_frame_time_ms = 0.0f;
};
