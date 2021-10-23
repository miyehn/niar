#pragma once
#include "Renderer.h"

class Texture2D;

class DeferredRenderer : public Renderer
{
protected:
	DeferredRenderer();
	~DeferredRenderer() override;

	VkRenderPass renderPass;
	VkFramebuffer framebuffer;

	VkExtent2D renderExtent;

public:

	Texture2D* GPosition;

	Texture2D* GNormal;

	Texture2D* GColor;

	Texture2D* GMetallicRoughnessAO;

	Texture2D* sceneColor;

	Texture2D* sceneDepth;

	void render() override;
	VkRenderPass getRenderPass() override { return renderPass; }

	static DeferredRenderer* get();
	static void cleanup();
};

