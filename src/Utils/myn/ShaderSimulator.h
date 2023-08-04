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


} // namespace sky
} // namespace myn

