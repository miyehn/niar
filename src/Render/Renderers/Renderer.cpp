//
// Created by miyehn on 5/9/2026.
//

#include "Renderer.h"
#include "Utils/myn/Sample.h"

Renderer::ViewInfo Renderer::getCameraViewInfo()
{
    ViewInfo viewInfo = {};

	// update whatever's needed
	viewInfo.ViewMatrix = camera->world_to_object();
	viewInfo.ProjectionMatrix = camera->camera_to_clip();
	viewInfo.ProjectionMatrix[1][1] *= -1; // so it's not upside down

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

    return viewInfo;
}