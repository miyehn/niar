//
// Created by raind on 5/24/2022.
//
#include "Utils/myn/Log.h"
#include "Scene/SceneObject.hpp"
#include "Pathtracer/Pathtracer.hpp"
#include "Assets/ConfigAsset.hpp"
#include "Assets/SceneAsset.h"
#include "Scene/SkyAtmosphere.h"
#include <cxxopts.hpp>
#include <windows.h>

int main(int argc, const char * argv[])
{
	cxxopts::Options options("aszelea", "pathtrace to file");
	options.allow_unrecognised_options();
	options.add_options()
		("w,width", "window width", cxxopts::value<int>())
		("h,height", "window height", cxxopts::value<int>())
		("o,output", "output relative_path", cxxopts::value<std::string>())
		("scene", "path to scene file (overrides SceneSource in global.ini)", cxxopts::value<std::string>())
		("spp", "samples per pixel (overrides MinRaysPerPixel in pathtracer.ini)", cxxopts::value<int>())
		("popup", "automatically open the result image when render finishes");

	auto optargs = options.parse(argc, argv);

	if (!optargs.count("output") || !optargs.count("width") || !optargs.count("height")) {
		ERR("required arguments not set.")
		return 0;
	}

	int width = optargs["width"].as<int>();
	int height = optargs["height"].as<int>();
	std::string output_path = optargs["output"].as<std::string>();

	// load config
	Config = new ConfigAsset("config/global.ini", false);

	// load scene
	std::string scene_source = optargs.count("scene")
		? optargs["scene"].as<std::string>()
		: Config->lookup<std::string>("SceneSource");
	auto scene_asset = new SceneAsset(nullptr, scene_source);

	// environment map
	if (Config->lookup<int>("LoadEnvironmentMap")) {
		new EnvironmentMapAsset(Config->lookup<std::string>("EnvironmentMap"));
	}

	// cleanup fn
	auto cleanup = []() {
		Asset::release_all();
		Asset::delete_all();
	};

	// find a camera and set it active
	Camera* camera = nullptr;
	scene_asset->get_root()->foreach_descendent_bfs([&camera](SceneObject* obj) {
		auto cam = dynamic_cast<Camera*>(obj);
		if (cam) camera = cam;
	});
	if (!camera) {
		ERR("there's no camera in the scene")
		cleanup();
		return 0;
	}

	// sky atmosphere
	auto sky = SkyAtmosphere::getInstance();
	scene_asset->get_root()->add_child(sky);
	sky->find_sun();

	auto pathtracer = Pathtracer::get(width, height);
	auto pathtracerConfig = pathtracer->get_config_ref();
	if (optargs.count("spp"))
		pathtracerConfig.MinRaysPerPixel = optargs["spp"].as<int>();
	pathtracer->drawable = scene_asset->get_root();
	pathtracer->camera = camera;

	LOG("rendering pathtracer scene to file: %s", output_path.c_str());
	pathtracer->render_to_file(output_path);

	if (optargs.count("popup"))
		ShellExecute(0, "open", output_path.c_str(), 0, 0, SW_SHOW);

	// cleanup

	cleanup();
	return 0;

}