//
// Created by miyehn on 11/10/2022.
//

#include "Utils/myn/Log.h"
#include "Utils/myn/ShaderSimulator.h"
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

	myn::sky::SkyAtmosphereSim sim(width, height, output_path);
	sim.runSim();

	return 0;
}
