#pragma once

#include "Render/RendererComponents/SceneTlas.h"
#include "Render/RendererComponents/SkyAtmosphereRender.h"
#include "Render/Vulkan/DescriptorSet.h"
#include "Render/Vulkan/Buffer.h"
#include "Renderer.h"

class Texture2D;
class ConfigAsset;

// Playground for path tracing experiments (MIS, RIS, ...) that runs entirely in one compute shader
// with ray queries, independent from the deferred renderer.
class GpuPathTracer : public Renderer
{

public:
	~GpuPathTracer();
	void render(VkCommandBuffer cmdbuf) override;
	void draw_config_ui() override;

	static GpuPathTracer* get();

private:

	GpuPathTracer();

	float cfgExposure = 3.0f;
	const ConfigAsset* config = nullptr;

	// Progressive accumulation: accumulationImage holds the average of the first sampleCount samples of every pixel.
	// It starts over whenever what it shows changes (the config reloads, or sceneStateHash changes).
	Texture2D* accumulationImage = nullptr;
	uint32_t sampleCount = 0;
	uint64_t sceneStateHash = 0;

	Texture2D* outImage = nullptr;
	SceneTlas sceneTlas;
	SkyAtmosphereRender skyAtmosphereRender;

	glm::PointLightInfo pointLights[MAX_LIGHTS_PER_PASS];
	glm::DirectionalLightInfo directionalLights[MAX_LIGHTS_PER_PASS];

	struct GpuFrameData {
		VmaBuffer viewInfoUbo;
		VmaBuffer pointLightsUbo;
		VmaBuffer directionalLightsUbo;
		DescriptorSet descriptorSet;
	};
	GpuFrameData gpuFrameData[MAX_FRAMES_IN_FLIGHT];

};
