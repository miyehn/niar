#pragma once
#include "Scene/Camera.hpp"
#include "Scene/Scene.hpp"
#if GRAPHICS_DISPLAY
#include "Render/Vulkan/Vulkan.hpp"
#endif

class Renderer
{
protected:

	// ViewInfo is optional for renderers, some don't use it (path tracer), but still common enough
	struct ViewInfo {
		glm::mat4 ViewMatrix;
		glm::mat4 ProjectionMatrix;

		glm::vec3 CameraPosition;
		int NumPointLights;

		glm::vec3 ViewDir;
		int NumDirectionalLights;

		float Exposure;
		float AspectRatio;
		float HalfVFovRadians;
		int ToneMappingOption;
		int BackgroundOption;
	};
	ViewInfo getCameraViewInfo();

	Renderer() = default;
public:

	virtual ~Renderer() = default;

	SceneObject* drawable = nullptr;
	Camera* camera = nullptr;

#if GRAPHICS_DISPLAY
	bool prevent_scene_reload = false;

	virtual void on_selected() {}
	virtual void on_unselected() {}

	virtual void draw_config_ui() {}

	virtual void render(VkCommandBuffer cmdbuf) = 0;
#else
	virtual void render_to_file(const std::string& output_path_rel_to_bin) = 0;
#endif
};