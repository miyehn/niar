#pragma once

#include "Renderer.h"
#include "Render/RendererComponents/SceneTlas.h"
#include "Render/RendererComponents/SkyAtmosphereRender.h"
#include "Render/Vulkan/DescriptorSet.h"
#include "Render/Vulkan/Buffer.h"
#include <vulkan/vulkan.h>

#include "Render/RendererComponents/GI.h"

class Texture2D;
class DebugPoints;
class DebugLines;
class DeferredLighting;
class PostProcessing;
class GltfMaterial;

#define GNORMAL_ATTACHMENT 0
#define GCOLOR_ATTACHMENT 1
#define GORM_ATTACHMENT 2

// main
#define DEFERRED_SUBPASS_GEOMETRY 0
// (gi goes in between)
#define DEFERRED_SUBPASS_LIGHTING 0
#define DEFERRED_SUBPASS_TRANSLUCENCY 1

// post process
#define DEFERRED_SUBPASS_POSTPROCESSING 0

// lighting related
#define MAX_LIGHTS_PER_PASS 128 // 4KB if each light takes { vec4, vec4 }. Must not exceed definition in shader.

enum BackgroundOption {
	BG_None,
	BG_EnvironmentMap,
	BG_SkyAtmosphere
};

class DeferredRenderer : public Renderer
{
public:

	DescriptorSetLayout getFrameGlobalLayout();
	DescriptorSet& getSkyDescriptorSet();

	VkRenderPass basePass;
	VkRenderPass lightingPass;
	VkRenderPass envmapVisualizationPass;
	VkRenderPass postProcessPass;
	VkRenderPass debugDrawPass;

	void render(VkCommandBuffer cmdbuf) override;

	void draw_config_ui() override;

	static DeferredRenderer* get();

private:

	DeferredRenderer();
	~DeferredRenderer() override;

	bool drawDebug = true;
	bool drawEnvmapVisualization = true;

	VkFramebuffer baseFramebuffer;
	VkFramebuffer lightingFramebuffer;
	VkFramebuffer envmapVisualizationFramebuffer; // sceneColor + sceneDepth
	VkFramebuffer postProcessFramebuffer; // postprocessed
	VkFramebuffer debugDrawFramebuffer; // postprocessed(RW) + sceneDepth(R)

	struct GpuFrameData {
		VmaBuffer viewInfoUbo;
		VmaBuffer pointLightsBuffer;
		VmaBuffer directionalLightsBuffer;
		DescriptorSet frameGlobalDescriptorSet;
		DebugPoints* debugPoints = nullptr;
		DebugLines* debugLines = nullptr;
	};
	GpuFrameData gpuFrameData[MAX_FRAMES_IN_FLIGHT];

	VkExtent2D renderExtent;

	Texture2D* GNormal;

	Texture2D* GColor;

	Texture2D* GORM;

	Texture2D* sceneColor;

	Texture2D* sceneDepth;

	Texture2D* postProcessed;

	// specific to this renderer

	DeferredLighting* deferredLighting;

	PostProcessing* postProcessing;

	float cfgExposure;
	int cfgToneMappingOption;

	// light buffers

	struct PointLightInfo {
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 color;
	};

	struct DirectionalLightInfo {
		alignas(16) glm::vec3 direction;
		alignas(16) glm::vec3 color;
	};

	struct {
		PointLightInfo Data[MAX_LIGHTS_PER_PASS]; // need to match shader
	} pointLights;

	struct {
		DirectionalLightInfo Data[MAX_LIGHTS_PER_PASS]; // need to match shader
	} directionalLights;

	// mesh materials

	std::unordered_map<std::string, GltfMaterial*> materials;
	GltfMaterial* getOrCreateMeshMaterial(const std::string& materialName);

	GI gi;
	SkyAtmosphereRender skyAtmosphereRender;
	// Shared TLAS for RTX shadows and GI.
	SceneTlas shadowTlas;
};
