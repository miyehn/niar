#pragma once

#include "Renderer.h"
#include "Render/Vulkan/DescriptorSet.h"

class Texture2D;
class DebugPoints;

class DeferredRenderer : public Renderer
{
private:
	DeferredRenderer();
	~DeferredRenderer() override;

	VkFramebuffer framebuffer;
	VkFramebuffer postProcessFramebuffer;

	VmaBuffer viewInfoUbo;

	VkExtent2D renderExtent;

	Texture2D* GPosition;

	Texture2D* GNormal;

	Texture2D* GColor;

	Texture2D* GMetallicRoughnessAO;

	Texture2D* sceneColor;

	Texture2D* sceneDepth;

	Texture2D* postProcessed;

	DebugPoints* debugPoints;

public:

	DescriptorSet frameGlobalDescriptorSet;

	VkRenderPass renderPass;
	VkRenderPass postProcessPass;

	struct
	{
		glm::mat4 ViewMatrix;
		glm::mat4 ProjectionMatrix;

		glm::vec3 CameraPosition;
		int NumPointLights;

		glm::vec3 ViewDir;
		int NumDirectionalLights;

		float Exposure;
		int ToneMappingOption;

	} ViewInfo;

	void render(VkCommandBuffer cmdbuf) override;

	static DeferredRenderer* get();
	static void cleanup();
};
