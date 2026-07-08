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

// Bakes a subpixel jitter (in pixels) into a projection matrix as a clip-space offset, so that
// every consumer of the resulting matrix (rasterization, and later screen-space reconstruction
// for lighting/shadows) sees a consistently jittered camera. This is equivalent to patching
// clipPos.xy += jitterClipSpace * clipPos.w post-multiply, folded into the matrix itself:
// row 0/1 get jitter.x/y times row 3 added, since clipPos.w = row3 . viewPos.
glm::mat4 applyPixelJitter(const glm::mat4& projectionMatrix, glm::vec2 jitterOffsetPixels, glm::vec2 renderSize)
{
	if (renderSize.x <= 0.0f || renderSize.y <= 0.0f || jitterOffsetPixels == glm::vec2(0.0f)) {
		return projectionMatrix;
	}
	const glm::vec2 jitterClipSpace = (jitterOffsetPixels / renderSize) * 2.0f;
	glm::mat4 jittered = projectionMatrix;
	for (int col = 0; col < 4; col++) {
		jittered[col][0] += jitterClipSpace.x * projectionMatrix[col][3];
		jittered[col][1] += jitterClipSpace.y * projectionMatrix[col][3];
	}
	return jittered;
}

} // namespace

glm::ViewInfo Renderer::getCameraViewInfo(glm::vec2 renderSize, bool applyJitter) const
{
    glm::ViewInfo viewInfo = {};

	// update whatever's needed
	viewInfo.ViewMatrix = camera->world_to_object();
	viewInfo.UnjitteredProjectionMatrix = camera->camera_to_clip();
	viewInfo.PrevViewMatrix = camera->previous_view_matrix();
	viewInfo.PrevUnjitteredProjectionMatrix = camera->previous_projection_matrix();

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

	viewInfo.RenderSize = renderSize;

	viewInfo.JitterOffset = applyJitter ? haltonJitterOffset(viewInfo.FrameIndex) : glm::vec2(0.0f);
	viewInfo.ProjectionMatrix = applyPixelJitter(viewInfo.UnjitteredProjectionMatrix, viewInfo.JitterOffset, renderSize);
	viewInfo.InverseProjectionMatrix = glm::inverse(viewInfo.ProjectionMatrix);

    return viewInfo;
}
