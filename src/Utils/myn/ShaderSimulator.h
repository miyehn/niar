//
// Created by miyehn on 11/10/2022.
//

#pragma once
#include <string>
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include "CpuTexture.h"

namespace myn
{
class ShaderSimulator {
public:
	explicit ShaderSimulator(CpuTexture* outputTexture);

	virtual void runSim();

protected:

	void dispatchShader(const std::function<glm::vec4(uint32_t, uint32_t)> &kernel);

	// note: shader does not own the texture(s).
	CpuTexture* output = nullptr;
};

namespace sky {

struct AtmosphereProfile {
	float bottomRadius;
	float topRadius;

	glm::vec3 rayleighScattering;
	// rayleigh absorption = 0

	glm::vec3 mieScattering;
	glm::vec3 mieAbsorption;

	float miePhaseG;

	// ozone scattering = 0
	glm::vec3 ozoneAbsorption;

	// ozone distribution
	float ozoneMeanHeight;
	float ozoneLayerWidth;

	glm::vec3 groundAlbedo;
};

struct SkyAtmosphereRenderingParams {
	AtmosphereProfile atmosphere;
	glm::vec3 cameraPosWS;
	glm::vec3 dir2sun;
	glm::vec3 sunLuminance;
	glm::vec2 skyViewNumSamplesMinMax;
	float exposure;
};

class SkyViewLutSim : public ShaderSimulator {
public:
	explicit SkyViewLutSim(CpuTexture* outputTexture)
	: ShaderSimulator(outputTexture) {}
	void runSim() override;

	const CpuTexture* transmittanceLut = nullptr;
	const SkyAtmosphereRenderingParams* renderingParams = nullptr;
};

class TransmittanceLutSim : public ShaderSimulator {
public:
	explicit TransmittanceLutSim(CpuTexture* outputTexture)
	: ShaderSimulator(outputTexture) {}
	void runSim() override;

	const AtmosphereProfile* atmosphere = nullptr;
};

class SkyAtmosphereSim : public ShaderSimulator {
public:
	explicit SkyAtmosphereSim(CpuTexture* outputTexture)
		: ShaderSimulator(outputTexture) {}
	void runSim() override;

	const CpuTexture* skyViewLut = nullptr;
	const SkyAtmosphereRenderingParams* renderingParams = nullptr;
};

class SkyAtmospherePostProcess : public ShaderSimulator {
public:
	explicit SkyAtmospherePostProcess(CpuTexture* outputTexture)
	: ShaderSimulator(outputTexture) {}
	void runSim() override;

	const CpuTexture* skyTextureRaw = nullptr;
	const SkyAtmosphereRenderingParams* renderingParams = nullptr;
};

} // namespace sky
} // namespace myn

