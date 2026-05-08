#pragma once

#include "Render/Vulkan/DescriptorSet.h"
#include "Render/Vulkan/ShaderBindingTable.h"
#include "Render/Vulkan/Buffer.h"
#include "Renderer.h"

class Texture2D;

class RayTracingRenderer : public Renderer
{
private:
	RayTracingRenderer();

public:
	~RayTracingRenderer();
	void render(VkCommandBuffer cmdbuf) override;

	static RayTracingRenderer* get();

	// Called once after RtxTriangle is constructed, to build the TLAS and
	// create the pipeline resources that depend on it.
	void setup(VkAccelerationStructureKHR blas);

	Texture2D* outImage = nullptr;

	VmaBuffer tlasBuffer;
	VkAccelerationStructureKHR tlas = VK_NULL_HANDLE;

	DescriptorSet descriptorSet;
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	ShaderBindingTable sbt;

private:
	VkExtent2D renderExtent{};
};

