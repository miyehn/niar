#pragma once
#include "cshared_common.h"
#include "Scene/Camera.hpp"
#include "Scene/Scene.hpp"
#if GRAPHICS_DISPLAY
#include "Render/Vulkan/Vulkan.hpp"
#endif

class Renderer
{
protected:

	// applyJitter should only be requested by a renderer that actually resolves the jitter
	// with a temporal filter (i.e. TAA); otherwise it just adds unresolved aliasing noise.
	[[nodiscard]] glm::ViewInfo getCameraViewInfo(glm::vec2 renderSize, bool applyJitter = false) const;

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
