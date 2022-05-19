#pragma once

#include "Renderer.h"
#include "Render/Vulkan/DescriptorSet.h"

class Texture2D;
class DebugPoints;
class DebugLines;
class DeferredLighting;
class Material;
class PostProcessing;

class DeferredRenderer : public Renderer
{
public:

	DebugPoints* debugPoints = nullptr;
	DebugLines* debugLines = nullptr;

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

	void draw_config_ui() override;

	static DeferredRenderer* get();

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

	// specific to this renderer

	DeferredLighting* deferredLighting;

	PostProcessing* postProcessing;

	void updateFrameGlobalDescriptorSet();

	static Material* getOrCreateMeshMaterial(const std::string& materialName);
};
