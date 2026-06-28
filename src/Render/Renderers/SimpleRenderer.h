#pragma once
#include "Render/Vulkan/DescriptorSet.h"
#include "DeferredRenderer.h"

class Texture2D;
class DebugLines;

class SimpleRenderer : public Renderer
{
public:

	void render(VkCommandBuffer cmdbuf) override;

	static SimpleRenderer* get();

	VkRenderPass renderPass;

	DescriptorSetLayout getFrameGlobalLayout();

private:

	struct GpuFrameData {
		VmaBuffer viewInfoUbo;
		DescriptorSet descriptorSet;
		DebugLines* debugLines = nullptr;
	};
	GpuFrameData gpuFrameData[MAX_FRAMES_IN_FLIGHT];

	SimpleRenderer();
	~SimpleRenderer() override;

	VkExtent2D renderExtent;
	Texture2D* sceneColor;
	Texture2D* sceneDepth;
	VkFramebuffer frameBuffer;

};
