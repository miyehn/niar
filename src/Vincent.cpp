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

#if 0
	myn::CpuTexture tex(width, height);
	myn::sky::TransmittanceLutSim sim(&tex);
	sim.runSim();

	myn::CpuTexture sampled(64, 64);
	myn::sky::SamplingTestSim sampleTest(&sampled);
	sampleTest.input = &tex;
	sampleTest.runSim();

	sampled.writeFile("sampled_transmittance.png", false, true);
#else

	TIMER_BEGIN

	// transmittance lut
	myn::CpuTexture transmittanceLut(192, 108);
	myn::sky::TransmittanceLutSim transmittanceSim(&transmittanceLut);
	transmittanceSim.runSim();

	// sky texture
	myn::CpuTexture skyTex(width, height);
	myn::sky::SkyAtmosphereSim skySim(&skyTex);
	skySim.transmittanceLut = &transmittanceLut;
	skySim.runSim();

	// post-processed sky texture
	myn::CpuTexture skyTexCopy(skyTex);
	myn::sky::SkyAtmospherePostProcess post(&skyTexCopy);
	post.input = &skyTex; // as read-only shader resource
	post.runSim();

	TIMER_END(simTime)

	LOG("done. took %.3f secs", simTime)

	skyTexCopy.writeFile(output_path, false, true);
#endif

	return 0;
}
