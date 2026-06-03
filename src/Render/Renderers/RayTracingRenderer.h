#pragma once

#include "Render/Vulkan/DescriptorSet.h"
#include "Render/Vulkan/ShaderBindingTable.h"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/Pipeline.h"
#include "Renderer.h"
#include "DeferredRenderer.h"

class Texture2D;

class RayTracingRenderer : public Renderer
{

public:
	~RayTracingRenderer();
	void render(VkCommandBuffer cmdbuf) override;

	static RayTracingRenderer* get();

private:

	RayTracingRenderer();

	Texture2D* outImage = nullptr;

	VkAccelerationStructureKHR tlas = VK_NULL_HANDLE;
	VmaBuffer tlasBuffer;
	VmaBuffer scratchBuffer;

	RayTracingPipeline rtPipeline;
	ShaderBindingTable sbt;

	struct GpuFrameData {
		VmaBuffer viewInfoUbo;
		DescriptorSet descriptorSet;
		VmaBuffer instancesBuffer;
	};
	GpuFrameData gpuFrameData[MAX_FRAMES_IN_FLIGHT];

	static constexpr uint32_t MAX_RTX_INSTANCES = 64;

};

