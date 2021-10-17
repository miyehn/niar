#pragma once
#include "Scene/Camera.hpp"
#include "Scene/Scene.hpp"
#include "Vulkan.hpp"

class Renderer
{
public:
	Renderer() = default;
	virtual ~Renderer() = default;

	std::vector<Drawable*> drawables;
	Camera* camera;

	virtual void render() = 0;
};

class SimpleRenderer : public Renderer
{
public:
	void render() override;
};