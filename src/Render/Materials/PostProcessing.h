#pragma once
#include "Material.h"
#include "Render/Renderers/DeferredRenderer.h"

class DeferredRenderer;
class Texture2D;

class PostProcessing : public Material
{
public:

	void usePipeline(VkCommandBuffer cmdbuf, std::vector<DescriptorSetBindingSlot> sharedDescriptorSets) override;

	static void cleanup();

private:

	// TODO: currently its construction is tied to the deferred renderer creation
	// but it doesn't have to.
	explicit PostProcessing(DeferredRenderer* renderer, Texture2D* sceneColor, Texture2D* sceneDepth);

	DescriptorSet dynamicSet;

	static VkPipeline pipeline;
	static VkPipelineLayout pipelineLayout;

	friend class DeferredRenderer;
};

