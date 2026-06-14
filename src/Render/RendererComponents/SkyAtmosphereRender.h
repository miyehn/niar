#pragma once

#include "Render/Materials/ComputeShader.h"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/DescriptorSet.h"
#include "Render/Vulkan/Vulkan.hpp"

class SkyAtmosphere;
class Texture2D;

struct SkyAtmosphereRender
{
	SkyAtmosphereRender() = default;
	SkyAtmosphereRender(const SkyAtmosphereRender&) = delete;
	SkyAtmosphereRender& operator=(const SkyAtmosphereRender&) = delete;

	void init();
	void release();
	void update_luts(SkyAtmosphere* sky);

	DescriptorSet& get_descriptor_set(const SkyAtmosphere* sky);

private:
	struct GpuFrameData {
		VmaBuffer parametersBuffer;
		DescriptorSet descriptorSet;
		DescriptorSet dummyDescriptorSet;
	};

	Texture2D* transmittanceLut = nullptr;
	Texture2D* skyViewLut = nullptr;
	GpuFrameData gpuFrameData[MAX_FRAMES_IN_FLIGHT];
};
