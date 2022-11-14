//
// Created by miyehn on 11/10/2022.
//

#include "Utils/myn/Log.h"
#include "Utils/myn/ShaderSimulator.h"
#include "Utils/myn/CpuTexture.h"
#include "Utils/myn/Timer.h"
#include <cxxopts/cxxopts.hpp>

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

	myn::sky::SkyAtmosphereRenderingParams params = {
		.cameraPosWS = glm::vec3(200, 300, 500),
		.dir2sun = glm::normalize(glm::vec3(1, 1, 0.2f)),
		.sunLuminance = {1, 1, 1},
		.skyViewNumSamplesMinMax = {32, 128},
		.exposure = 10
	};

	auto& atmosphere = params.atmosphere;
	float mieScattering = 0.003996f;
	float mieExtinction = 0.00440f;
	float mieAbsorption = mieExtinction - mieScattering;
	atmosphere = {
		.bottomRadius = 6360,
		.topRadius = 6460,
		.rayleighScattering = {
			5.802f * 1e-3,
			13.558f * 1e-3,
			33.1f * 1e-3
		},
		// rayleigh absorption = 0
		.mieScattering = {
			mieScattering, mieScattering, mieScattering
		},
		.mieAbsorption = {
			mieAbsorption, mieAbsorption, mieAbsorption
		},
		.miePhaseG = 0.8f,
		// ozone scattering = 0
		.ozoneAbsorption = {
			0.650f * 1e-3,
			1.881f * 1e-3,
			0.085f * 1e-3
		},
		.ozoneMeanHeight = 25,
		.ozoneLayerWidth = 30,
		.groundAlbedo = {0.3f, 0.3f, 0.3f}
	};

	// transmittance lut
	myn::CpuTexture transmittanceLut(256, 64);
	myn::sky::TransmittanceLutSim transmittanceSim(&transmittanceLut);
	transmittanceSim.atmosphere = &atmosphere;
	transmittanceSim.runSim();

	// sky view lut
	myn::CpuTexture skyViewLut(192, 108);
	myn::sky::SkyViewLutSim skyViewSim(&skyViewLut);
	skyViewSim.transmittanceLut = &transmittanceLut;
	skyViewSim.renderingParams = &params;
	skyViewSim.runSim();

	// post-processed sky texture
	//myn::CpuTexture skyTextureRaw(width, height);
	// todo: transform sky view lut into a long-lat map

	myn::CpuTexture skyTexturePostProcessed(width, height);
	myn::sky::SkyAtmospherePostProcess post(&skyTexturePostProcessed);
	post.skyTextureRaw = &skyViewLut; // as read-only shader resource
	post.renderingParams = &params;
	post.runSim();

	skyTexturePostProcessed.writeFile("debug.png", false, true);

	return 0;
}
