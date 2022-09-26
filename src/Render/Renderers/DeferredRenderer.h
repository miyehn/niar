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
#define GORM_ATTACHMENT 3
#define SCENECOLOR_ATTACHMENT 4
#define SCENEDEPTH_ATTACHMENT 5

// main
#define DEFERRED_SUBPASS_GEOMETRY 0
#define DEFERRED_SUBPASS_LIGHTING 1
#define DEFERRED_SUBPASS_PROBES 2
#define DEFERRED_SUBPASS_TRANSLUCENCY 3

// post process
#define DEFERRED_SUBPASS_POSTPROCESSING 0
#define DEFERRED_SUBPASS_DEBUGDRAW 1

// lighting related
#define MAX_LIGHTS_PER_PASS 128 // 4KB if each light takes { vec4, vec4 }. Must not exceed definition in shader.

struct ViewInfo {
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

};

class DeferredRenderer : public Renderer
{
public:

	DebugPoints* debugPoints = nullptr;
	DebugLines* debugLines = nullptr;

	VmaBuffer pointLightsBuffer; // move to renderer
	VmaBuffer directionalLightsBuffer; // move to renderer

	DescriptorSet frameGlobalDescriptorSet;

	VkRenderPass mainPass;
	VkRenderPass postProcessPass;

	ViewInfo viewInfo;

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

	Texture2D* GORM;

	Texture2D* sceneColor;

	Texture2D* sceneDepth;

	Texture2D* postProcessed;

	// specific to this renderer

	DeferredLighting* deferredLighting;

	PostProcessing* postProcessing;

	void updateUniformBuffers();

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
	Material* getOrCreateMeshMaterial(const std::string& materialName);
};
