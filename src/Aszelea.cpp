//
// Created by raind on 5/24/2022.
//
#include "Utils/myn/Log.h"
#include "Scene/SceneObject.hpp"
#include "Pathtracer/Pathtracer.hpp"
#include "Assets/ConfigAsset.hpp"
#include "Assets/GltfAsset.h"
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

	// load config
	Config = new ConfigAsset("config/global.ini", false);

	// load scene
	auto scene_asset = new GltfAsset(
		nullptr,
		Config->lookup<std::string>("SceneSource"));

	// cleanup fn
	auto cleanup = [scene_asset]() {
		releaseAllAssets();
		delete scene_asset;
		delete Config;
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

	auto pathtracer = Pathtracer::get(width, height);
	pathtracer->drawable = scene_asset->get_root();
	pathtracer->camera = camera;

	LOG("rendering pathtracer scene to file: %s", output_path.c_str());
	pathtracer->render_to_file(output_path);

	// cleanup

	cleanup();
	return 0;

}