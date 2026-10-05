#pragma once

#include "Render/RendererComponents/SceneTlas.h"
#include "Render/Vulkan/DescriptorSet.h"
#include "Render/Vulkan/Buffer.h"
#include "Renderer.h"

class Texture2D;

// Playground for path tracing experiments (MIS, RIS, ...) that runs entirely in one compute shader
// with ray queries, independent from the deferred renderer. Opaque geometry only.
class GpuPathTracer : public Renderer
{

public:
	~GpuPathTracer();
	void render(VkCommandBuffer cmdbuf) override;

	static GpuPathTracer* get();

private:

	GpuPathTracer();

	Texture2D* outImage = nullptr;
	SceneTlas sceneTlas;

	struct GpuFrameData {
		VmaBuffer viewInfoUbo;
		DescriptorSet descriptorSet;
	};
	GpuFrameData gpuFrameData[MAX_FRAMES_IN_FLIGHT];

};
