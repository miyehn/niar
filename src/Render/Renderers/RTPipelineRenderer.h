#pragma once

#include "Render/RendererComponents/SceneTlas.h"
#include "Render/Vulkan/DescriptorSet.h"
#include "Render/Vulkan/ShaderBindingTable.h"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/Pipeline.h"
#include "Renderer.h"

class Texture2D;

class RTPipelineRenderer : public Renderer
{

public:
	~RTPipelineRenderer();
	void render(VkCommandBuffer cmdbuf) override;

	static RTPipelineRenderer* get();

private:

	RTPipelineRenderer();

	Texture2D* outImage = nullptr;
	SceneTlas sceneTlas;

	RayTracingPipeline rtPipeline;
	ShaderBindingTable sbt;

	struct GpuFrameData {
		VmaBuffer viewInfoUbo;
		DescriptorSet descriptorSet;
	};
	GpuFrameData gpuFrameData[MAX_FRAMES_IN_FLIGHT];

};

