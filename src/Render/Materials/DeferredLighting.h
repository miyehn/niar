#pragma once
#include "Material.h"
#include "Render/Renderers/DeferredRenderer.h"

#define MAX_LIGHTS_PER_PASS 128 // 4KB if each light takes { vec4, vec4 }. Must not exceed definition in shader.

class DeferredRenderer;

class DeferredLighting : public Material
{
public:

	MaterialPipeline getPipeline() override;
	void usePipeline(VkCommandBuffer cmdbuf) override;
	~DeferredLighting() override;

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

	// optimization, set by renderer to decide how much data to actually write to uniform buffers
	uint32_t numPointLights, numDirectionalLights;

private:

	explicit DeferredLighting(DeferredRenderer* renderer);

	VmaBuffer pointLightsBuffer;
	VmaBuffer directionalLightsBuffer;

	DescriptorSet dynamicSet;
	VkRenderPass mainRenderPass;

	friend class DeferredRenderer;
};