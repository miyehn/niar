//
// Created by miyehn on 5/9/2026.
//

#include "Renderer.h"

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

    return viewInfo;
}