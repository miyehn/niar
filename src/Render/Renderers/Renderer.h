#pragma once
#include "Scene/Camera.hpp"
#include "Scene/Scene.hpp"
#include "Render/Vulkan/Vulkan.hpp"
#include <functional>

class Renderer
{
protected:
	Renderer() = default;
	virtual ~Renderer() = default;
public:

	SceneObject* drawable = nullptr;
	Camera* camera = nullptr;

	virtual void render(VkCommandBuffer cmdbuf) = 0;
};