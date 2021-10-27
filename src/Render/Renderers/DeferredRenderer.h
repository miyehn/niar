#pragma once
#include "Renderer.h"
#include "Render/Vulkan/DescriptorSet.h"

class Texture2D;

class DeferredRenderer : public Renderer
{
protected:
	DeferredRenderer();
	~DeferredRenderer() override;

	VkRenderPass renderPass;
	VkFramebuffer framebuffer;
	VmaBuffer viewInfoUbo;

	VkExtent2D renderExtent;

	Texture2D* GPosition;

	Texture2D* GNormal;

	Texture2D* GColor;

	Texture2D* GMetallicRoughnessAO;

	Texture2D* sceneColor;

	Texture2D* sceneDepth;

public:

	DescriptorSet frameGlobalDescriptorSet;

	struct
	{
		glm::mat4 ViewMatrix;
		glm::mat4 ProjectionMatrix;

		glm::vec3 CameraPosition;
		int NumPointLights;

		glm::vec3 ViewDir;
		int NumDirectionalLights;

	} ViewInfo;

	void render(VkCommandBuffer cmdbuf) override;
	VkRenderPass getRenderPass() override { return renderPass; }

	static DeferredRenderer* get();
	static void cleanup();
};

