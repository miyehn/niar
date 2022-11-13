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
	ShaderSimulator(int width, int height);

	virtual void runSim();

	const CpuTexture& getOutputTexture() { return outputTexture; }

protected:

	void dispatchShader(const std::function<glm::vec4(uint32_t, uint32_t)> &kernel);

	CpuTexture outputTexture;
};

namespace sky {

class SkyAtmosphereSim : public ShaderSimulator {
public:
	SkyAtmosphereSim(int width, int height)
		: ShaderSimulator(width, height) {}

	void runSim() override;
};

} // namespace sky
} // namespace myn

