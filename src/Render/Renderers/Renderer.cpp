//
// Created by miyehn on 5/9/2026.
//

#include "Renderer.h"
#include "Utils/myn/Sample.h"

namespace
{

float haltonSequence(uint32_t index, uint32_t base)
{
	float f = 1.0f;
	float result = 0.0f;
	while (index > 0) {
		f /= static_cast<float>(base);
		result += f * static_cast<float>(index % base);
		index /= base;
	}
	return result;
}

// 8-sample Halton(2, 3) sequence, recentered to [-0.5, 0.5] pixel offsets
glm::vec2 haltonJitterOffset(uint32_t frameIndex)
{
	constexpr uint32_t period = 8;
	uint32_t i = (frameIndex % period) + 1; // 1-indexed: halton(0, base) is always 0
	return glm::vec2(haltonSequence(i, 2) - 0.5f, haltonSequence(i, 3) - 0.5f);
}

} // namespace

glm::ViewInfo Renderer::getCameraViewInfo() const
{
    glm::ViewInfo viewInfo = {};

	// update whatever's needed
	viewInfo.ViewMatrix = camera->world_to_object();
	viewInfo.ProjectionMatrix = camera->camera_to_clip();
	viewInfo.InverseProjectionMatrix = glm::inverse(viewInfo.ProjectionMatrix);
	viewInfo.PrevViewMatrix = camera->previous_view_matrix();
	viewInfo.PrevProjectionMatrix = camera->previous_projection_matrix();

	viewInfo.CameraPosition = camera->world_position();
	viewInfo.ViewDir = camera->forward();

	viewInfo.AspectRatio = camera->aspect_ratio;
	viewInfo.HalfVFovRadians = camera->fov * 0.5f;

#if GRAPHICS_DISPLAY
	viewInfo.FrameIndex = Vulkan::Instance->getGlobalFrameIndex();
#else
	viewInfo.FrameIndex = 0;
#endif

	viewInfo.FrameRandom = glm::vec4(
		myn::sample::rand01(),
		myn::sample::rand01(),
		myn::sample::rand01(),
		myn::sample::rand01()
		);

	viewInfo.RenderSize = glm::vec2(); // specific renderer should fill this out if want to use it

	viewInfo.JitterOffset = haltonJitterOffset(viewInfo.FrameIndex);

    return viewInfo;
}
