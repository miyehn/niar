#pragma once

#include "Renderer.h"

class RayTracingRenderer : public Renderer
{
private:
	RayTracingRenderer();
	~RayTracingRenderer() override;

public:
	void render(VkCommandBuffer cmdbuf) override;

	void debugSetup(std::function<void()> fn) override;

	static RayTracingRenderer* get();
};

