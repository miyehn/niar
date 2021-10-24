#pragma once
#include "Scene/Camera.hpp"
#include "Scene/Scene.hpp"
#include "Render/Vulkan/Vulkan.hpp"

class Renderer
{
protected:
	Renderer() = default;
	virtual ~Renderer() = default;
public:

	std::vector<Drawable*> drawables;
	Camera* camera = nullptr;

	virtual void render() = 0;
	virtual VkRenderPass getRenderPass() = 0;
};