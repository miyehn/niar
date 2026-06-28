#include "FpsMeter.h"

#include "Render/Vulkan/Vulkan.hpp"

#include <algorithm>

#if GRAPHICS_DISPLAY
#include <imgui.h>
#endif

void FpsMeter::beginFrame()
{
	cpu_frame_start = std::chrono::high_resolution_clock::now();
}

void FpsMeter::endFrame(float elapsed)
{
	myn::TimePoint cpu_frame_end = std::chrono::high_resolution_clock::now();
	float cpu_elapsed_ms = std::chrono::duration<float, std::milli>(cpu_frame_end - cpu_frame_start).count();
	cpu_elapsed_ms = std::max(0.0f, cpu_elapsed_ms - Vulkan::Instance->getLastFrameWaitTimeMs());

	float gpu_elapsed_ms = 0.0f;
	bool has_gpu_elapsed_ms = Vulkan::Instance->getLastGpuFrameTimeMs(gpu_elapsed_ms);

	accumulated_time += elapsed;
	accumulated_cpu_frame_time_ms += cpu_elapsed_ms;
	accumulated_frames++;
	if (has_gpu_elapsed_ms) {
		accumulated_gpu_frame_time_ms += gpu_elapsed_ms;
		accumulated_gpu_frames++;
	}

	if (accumulated_time >= update_interval)
	{
		fps = accumulated_frames / accumulated_time;
		cpu_frame_time_ms = accumulated_cpu_frame_time_ms / accumulated_frames;
		if (accumulated_gpu_frames > 0) {
			gpu_frame_time_ms = accumulated_gpu_frame_time_ms / accumulated_gpu_frames;
		}
		accumulated_time = 0.0f;
		accumulated_cpu_frame_time_ms = 0.0f;
		accumulated_gpu_frame_time_ms = 0.0f;
		accumulated_frames = 0;
		accumulated_gpu_frames = 0;
	}
}

void FpsMeter::draw() const
{
#if GRAPHICS_DISPLAY
	ImGui::Text("FPS: %.1f (CPU: %.2fms, GPU: %.2fms)", fps, cpu_frame_time_ms, gpu_frame_time_ms);
#endif
}
