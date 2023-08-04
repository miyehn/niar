//
// Created by miyehn on 8/3/2023.
//

#pragma once

#include <string>
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include "Utils/myn/ShaderSimulator.h"

namespace myn::sky {

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
	float sunAngularRadius;
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

	AtmosphereProfile atmosphere;
};

class SkyAtmosphereSim : public ShaderSimulator {
public:
	explicit SkyAtmosphereSim(CpuTexture* outputTexture)
		: ShaderSimulator(outputTexture) {}
	void runSim() override;

	const CpuTexture* transmittanceLut = nullptr;
	const CpuTexture* skyViewLut = nullptr;
	SkyAtmosphereRenderingParams renderingParams;
};

class SkyAtmospherePostProcess : public ShaderSimulator {
public:
	explicit SkyAtmospherePostProcess(CpuTexture* outputTexture)
	: ShaderSimulator(outputTexture) {}
	void runSim() override;

	const CpuTexture* skyTextureRaw = nullptr;
	SkyAtmosphereRenderingParams renderingParams;
};

class CpuSkyAtmosphere {
public:
	CpuSkyAtmosphere();

	glm::vec3 sampleSkyColor(const glm::vec3& viewDir);

	glm::vec3 sampleSunTransmittance(const glm::vec3& viewDir) const;

	void updateLuts();

	CpuTexture createSkyTexture(int width, int height);

	SkyAtmosphereRenderingParams renderingParams;

private:

	CpuTexture transmittanceLut;
	CpuTexture skyViewLut;

};

}

