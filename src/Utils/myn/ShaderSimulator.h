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

	// note: shader does not own the output.
	CpuTexture* output = nullptr;
};

namespace sky {

class DebugTestSim : public ShaderSimulator {
public:
	explicit DebugTestSim(CpuTexture* outputTexture)
	: ShaderSimulator(outputTexture) {}

	void runSim() override;

	const CpuTexture* input = nullptr;
};

class TransmittanceLutSim : public ShaderSimulator {
public:
	explicit TransmittanceLutSim(CpuTexture* outputTexture)
	: ShaderSimulator(outputTexture) {}

	void runSim() override;
};

class SkyAtmosphereSim : public ShaderSimulator {
public:
	explicit SkyAtmosphereSim(CpuTexture* outputTexture)
		: ShaderSimulator(outputTexture) {}

	void runSim() override;

	const CpuTexture* transmittanceLut = nullptr;
};

class SkyAtmospherePostProcess : public ShaderSimulator {
public:
	explicit SkyAtmospherePostProcess(CpuTexture* outputTexture)
	: ShaderSimulator(outputTexture) {}

	void runSim() override;

	const CpuTexture* input = nullptr;
};

} // namespace sky
} // namespace myn

