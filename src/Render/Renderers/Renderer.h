#pragma once
#include "Scene/Camera.hpp"
#include "Scene/Scene.hpp"
#include "Render/Vulkan/Vulkan.hpp"
#include <functional>

class Renderer
{
protected:
	Renderer() = default;
public:

	virtual ~Renderer() = default;

	SceneObject* drawable = nullptr;
	Camera* camera = nullptr;

	bool prevent_scene_reload = false;

	virtual void on_selected() {}
	virtual void on_unselected() {}

	virtual void draw_config_ui() {}

	virtual void render(VkCommandBuffer cmdbuf) = 0;
};