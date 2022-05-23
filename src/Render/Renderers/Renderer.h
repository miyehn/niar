#pragma once
#include "Scene/Camera.hpp"
#include "Scene/Scene.hpp"
#include <functional>
#if GRAPHICS_DISPLAY
#include "Render/Vulkan/Vulkan.hpp"
#endif

class Renderer
{
protected:
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
	virtual void render_to_file(uint32_t w, uint32_t h, const std::string& path) = 0;
#endif
};