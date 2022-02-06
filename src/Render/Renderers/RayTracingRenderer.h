#pragma once

#include "Render/Vulkan/DescriptorSet.h"
#include "Renderer.h"

class Texture2D;

class RayTracingRenderer : public Renderer
{
private:
	RayTracingRenderer();
	~RayTracingRenderer() override;

public:
	void render(VkCommandBuffer cmdbuf) override;

	void debugSetup(std::function<void()> fn) override;

	static RayTracingRenderer* get();

private:
	VkExtent2D renderExtent;
	Texture2D* sceneColor;
	VkFramebuffer frameBuffer;
	VkRenderPass renderPass;

	DescriptorSet descriptorSet;
	VmaBuffer uniformBuffer;

public:
	struct {
		glm::vec4 color;
	} Uniforms;
};

