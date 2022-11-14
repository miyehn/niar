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

#if 1
	// transmittance lut
	myn::CpuTexture transmittanceLut(192, 108);
	myn::sky::TransmittanceLutSim transmittanceSim(&transmittanceLut);
	transmittanceSim.runSim();

	myn::CpuTexture tex(256, 128);
	myn::sky::DebugTestSim sim(&tex);
	sim.input = &transmittanceLut;
	sim.runSim();

	// post-processed sky texture
	myn::CpuTexture skyTexCopy(tex);
	myn::sky::SkyAtmospherePostProcess post(&skyTexCopy);
	post.input = &tex; // as read-only shader resource
	post.runSim();

	skyTexCopy.writeFile("debug.png", false, true);
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
