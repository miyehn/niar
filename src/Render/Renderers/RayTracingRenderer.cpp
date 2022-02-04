//
// Created by raind on 1/29/2022.
//

#include "RayTracingRenderer.h"

RayTracingRenderer::RayTracingRenderer()
{

}

RayTracingRenderer::~RayTracingRenderer()
{

}

void RayTracingRenderer::render(VkCommandBuffer cmdbuf)
{

}

RayTracingRenderer *RayTracingRenderer::get()
{
	static RayTracingRenderer* renderer = nullptr;
	if (renderer == nullptr)
	{
		renderer = new RayTracingRenderer();
		Vulkan::Instance->destructionQueue.emplace_back([](){ delete renderer; });
	}
	return renderer;
}

void RayTracingRenderer::debugSetup(std::function<void()> fn)
{
	if (fn) fn();
}
