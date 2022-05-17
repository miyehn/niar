#pragma once
#include "Material.h"
#include "Render/Renderers/DeferredRenderer.h"

class DeferredRenderer;
class Texture2D;

class PostProcessing : public Material
{
public:

	MaterialPipeline getPipeline() override;
	void usePipeline(VkCommandBuffer cmdbuf) override;

private:

	// TODO: currently its construction is tied to the deferred renderer creation
	// but it doesn't have to.
	explicit PostProcessing(DeferredRenderer* renderer, Texture2D* sceneColor, Texture2D* sceneDepth);

	VkRenderPass postProcessPass;
	DescriptorSet dynamicSet;

	friend class DeferredRenderer;
};

