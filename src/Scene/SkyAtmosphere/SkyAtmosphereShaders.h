//
// Created by miyehn on 11/17/2022.
//

#pragma once
#include "Render/Materials/ComputeShader.h"

#define CS_GROUPSIZE_X 8
#define CS_GROUPSIZE_Y 8

// checklist: https://community.khronos.org/t/drawing-to-image-from-compute-shader-example/7116/2
class TransmittanceLutCS : public ComputeShader {
public:
	VkImage targetImage = VK_NULL_HANDLE;
	void dispatch(int groupCountX, int groupCountY, int groupCountZ) override;
private:
	explicit TransmittanceLutCS() {
		shaderPath = "spirv/sky_transmittance_lut.comp.spv";
	}
	friend class ComputeShader;
};

class SkyViewLutCS : public ComputeShader {
public:
	// params
	VkImage targetImage = VK_NULL_HANDLE;
	// dispatch fn
	void dispatch(int groupCountX, int groupCountY, int groupCountZ) override;

private:
	explicit SkyViewLutCS() {
		shaderPath = "spirv/sky_view_lut.comp.spv";
	}
	friend class ComputeShader;
};
