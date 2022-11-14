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
		.cameraPosWS = glm::vec3(200, 300, 1000),
		.dir2sun = glm::normalize(glm::vec3(0, 0, 1)),
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

	double totalTime = 0;

	// transmittance lut
	myn::CpuTexture transmittanceLut(256, 64);
	{
		TIMER_BEGIN
		myn::sky::TransmittanceLutSim transmittanceSim(&transmittanceLut);
		transmittanceSim.atmosphere = &atmosphere;
		transmittanceSim.runSim();
		TIMER_END(tTransmittance)
		LOG("transmittance lut: %.3f", tTransmittance)
		totalTime += tTransmittance;
	}

	// sky view lut
	myn::CpuTexture skyViewLut(192, 108);
	{
		TIMER_BEGIN
		myn::sky::SkyViewLutSim skyViewSim(&skyViewLut);
		skyViewSim.transmittanceLut = &transmittanceLut;
		skyViewSim.renderingParams = &params;
		skyViewSim.runSim();
		TIMER_END(tSkyView)
		LOG("sky view lut: %.3f", tSkyView)
		totalTime += tSkyView;
	}

	// compositing
	myn::CpuTexture skyTextureRaw(width, height);
	{
		TIMER_BEGIN
		myn::sky::SkyAtmosphereSim mainSim(&skyTextureRaw);
		mainSim.renderingParams = &params;
		mainSim.skyViewLut = &skyViewLut;
		mainSim.runSim();
		TIMER_END(tComposite)
		LOG("compositing: %.3f", tComposite)
		totalTime += tComposite;
	}

	// post-processed sky texture
	myn::CpuTexture skyTexturePostProcessed(width, height);
	{
		TIMER_BEGIN
		myn::sky::SkyAtmospherePostProcess post(&skyTexturePostProcessed);
		post.skyTextureRaw = &skyTextureRaw; // as read-only shader resource
		post.renderingParams = &params;
		post.runSim();
		TIMER_END(tPostProcess)
		LOG("post processing: %.3f", tPostProcess)
		totalTime += tPostProcess;
	}

	LOG("total: %.3f", totalTime)

	skyTexturePostProcessed.writeFile("debug.png", false, true);

	return 0;
}
