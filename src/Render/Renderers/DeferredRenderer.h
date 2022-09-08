#pragma once

#include "Renderer.h"
#include "Render/Vulkan/DescriptorSet.h"

class Texture2D;
class DebugPoints;
class DebugLines;
class DeferredLighting;
class Material;
class PostProcessing;
class GltfMaterial;

#define GPOSITION_ATTACHMENT 0
#define GNORMAL_ATTACHMENT 1
#define GCOLOR_ATTACHMENT 2
#define GMETALLICROUGHNESSAO_ATTACHMENT 3
#define SCENECOLOR_ATTACHMENT 4
#define SCENEDEPTH_ATTACHMENT 5

// opaque
#define DEFERRED_SUBPASS_GEOMETRY 0
#define DEFERRED_SUBPASS_LIGHTING 1
#define DEFERRED_SUBPASS_PROBES 2

// post process
#define DEFERRED_SUBPASS_POSTPROCESSING 0
#define DEFERRED_SUBPASS_DEBUGDRAW 1

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
		float AspectRatio;
		float HalfVFovRadians;
		int ToneMappingOption;
		int UseEnvironmentMap;

	} ViewInfo;

	void render(VkCommandBuffer cmdbuf) override;

	void draw_config_ui() override;

	static DeferredRenderer* get();

private:

	DeferredRenderer();
	~DeferredRenderer() override;

	bool drawDebug = true;

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

	void updateViewInfoUbo();

	// mesh materials

	std::unordered_map<std::string, GltfMaterial*> materials;
	Material* getOrCreateMeshMaterial(const std::string& materialName);
};
