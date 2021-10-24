#pragma once
#include "Material.h"
#include "Render/Renderers/DeferredRenderer.h"

#define MAX_LIGHTS_PER_PASS 4

class MatDeferredPointLighting : public Material
{
public:

	explicit MatDeferredPointLighting();
	void usePipeline(VkCommandBuffer cmdbuf, std::vector<DescriptorSetBindingSlot> sharedDescriptorSets={}) override;
	~MatDeferredPointLighting() override;

	struct PointLightInfo {
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 color;
	};

	struct
	{
		PointLightInfo Lights[MAX_LIGHTS_PER_PASS]; // need to match shader
	} uniforms;

private:

	VmaBuffer uniformBuffer;

	DescriptorSet dynamicSet;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};