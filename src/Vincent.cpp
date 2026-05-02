//
// Created by miyehn on 11/10/2022.
//

#include "Utils/myn/Log.h"
#include "CpuSkyAtmosphere/CpuSkyAtmosphere.h"
#include "Utils/myn/CpuTexture.h"
#include "Utils/myn/Timer.h"
#include "Utils/myn/ShaderSimulator.h"
#include <cxxopts.hpp>
#include <random>

class ShaderTemp : public myn::ShaderSimulator {
public:
	explicit ShaderTemp(myn::CpuTexture* outTexture) : myn::ShaderSimulator(outTexture) {}
	void runSim() override {
		auto texdim = glm::uvec2(output->getWidth(), output->getHeight());
		std::mt19937 gen(1);
		std::uniform_real_distribution<float> dis(0.0f, 1.0f);
		dispatchShader([&](uint32_t x, uint32_t y){
			float r = dis(gen);
			return glm::vec4(r, r, r, 1);
		}, false);
	}
};

void GenerateTextureTemp(const std::string &output_path) {
	myn::CpuTexture tempTexture = myn::CpuTexture(32, 32);
	ShaderTemp shader(&tempTexture);
	shader.runSim();
	tempTexture.writeFile(output_path, false, true);
}

int main(int argc, const char * argv[])
{
	cxxopts::Options options("aszelea", "pathtrace to file");
	options.allow_unrecognised_options();
	options.add_options()
		("w,width", "window width", cxxopts::value<int>())
		("h,height", "window height", cxxopts::value<int>())
		("o,output", "output relative_path", cxxopts::value<std::string>());

	auto optargs = options.parse(argc, argv);

	if (!optargs.count("output") || !optargs.count("width") || !optargs.count("height")) {
		ERR("required arguments not set.")
		return 0;
	}

	int width = optargs["width"].as<int>();
	int height = optargs["height"].as<int>();
	std::string output_path = optargs["output"].as<std::string>();

	LOG("output path: %s", output_path.c_str())

	/////////////////////////////////////////////////////////////////////////////

	/*
	myn::sky::CpuSkyAtmosphere sky;
	sky.updateLuts();
	myn::CpuTexture skyTexture = sky.createSkyTexture(width, height);
	skyTexture.writeFile(output_path, false, true);
	 */
	GenerateTextureTemp(output_path);

	return 0;
}
