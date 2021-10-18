#pragma once
#include "Renderer.h"

class DeferredRenderer : public Renderer
{
protected:
	DeferredRenderer();
	~DeferredRenderer() override;

	VmaAllocatedImage GPosition;
	VkImageView GPositionView;

	VmaAllocatedImage GNormal;
	VkImageView GNormalView;

	VmaAllocatedImage GColor;
	VkImageView GColorView;

	VmaAllocatedImage GMetallicRoughnessAO;
	VkImageView GMetallicRoughnessAOView;

	VmaAllocatedImage sceneColor;
	VkImageView sceneColorView;

	VmaAllocatedImage sceneDepth;
	VkImageView sceneDepthView;

	VkRenderPass renderPass;
	VkFramebuffer framebuffer;

	VkExtent2D renderExtent;

public:
	void render() override;
	VkRenderPass getRenderPass() override { return renderPass; }

	static DeferredRenderer* get();
	static void cleanup();
};

