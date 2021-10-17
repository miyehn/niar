#pragma once
#include "Scene/Camera.hpp"
#include "Scene/Scene.hpp"
#include "Vulkan.hpp"

class Renderer
{
protected:
	Renderer() = default;
	virtual ~Renderer() = default;
public:

	std::vector<Drawable*> drawables;
	Camera* camera = nullptr;

	virtual void render() = 0;
};

class SimpleRenderer : public Renderer
{
public:
	void render() override;
};

class AnotherRenderer : public Renderer
{
protected:
	AnotherRenderer();
	~AnotherRenderer() override;

	// need to be responsible for tracking the intermediate image attachments, framebuffers, renderpasses
	VmaAllocatedImage outColor;
	VkImageView outColorView;

	VmaAllocatedImage outDepth;
	VkImageView outDepthView;

	VkRenderPass intermediatePass;
	VkFramebuffer intermediateFB;

	VkExtent2D swapChainExtent;

public:
	void render() override;
	VkRenderPass getRenderPass() { return intermediatePass; }

	static AnotherRenderer* get();
	static void cleanup();
};