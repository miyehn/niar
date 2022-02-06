#include "Config.hpp"
#include "Render/Mesh.h"
#include "Render/Texture.h"
#include "Render/Materials/DeferredBasepass.h"
#include "Utils/myn/Log.h"
#include "libconfig/libconfig.h++"

using namespace libconfig;

ProgramConfig Cfg;

void create_config_src(const std::string &path, Config& out_config_src)
{
	try {
		out_config_src.readFile(path.c_str());
	} catch (const FileIOException &fioex) {
		ERR("I/O error while reading config.ini");
		return;
	} catch (const ParseException &pex) {
		ERR("Config file parse error at %s:%d - %s", pex.getFile(), pex.getLine(), pex.getError());
		return;
	}

}

void initialize_pathtracer_config()
{
	Config config_src;
	create_config_src(Cfg.PathtracerConfigSource, config_src);

	try {
		// pathtracer
		Cfg.Pathtracer.ISPC = config_src.lookup("Pathtracer.ISPC");
		Cfg.Pathtracer.UseBVH->set(config_src.lookup("Pathtracer.UseBVH"));
		Cfg.Pathtracer.SmallWindow = config_src.lookup("Pathtracer.SmallWindow");
		Cfg.Pathtracer.Multithreaded = config_src.lookup("Pathtracer.Multithreaded");
		Cfg.Pathtracer.NumThreads = config_src.lookup("Pathtracer.NumThreads");
		Cfg.Pathtracer.TileSize = config_src.lookup("Pathtracer.TileSize");
		Cfg.Pathtracer.UseDirectLight = config_src.lookup("Pathtracer.UseDirectLight");
		Cfg.Pathtracer.UseJitteredSampling = config_src.lookup("Pathtracer.UseJitteredSampling");
		Cfg.Pathtracer.UseDOF->set(config_src.lookup("Pathtracer.UseDOF"));
		Cfg.Pathtracer.FocalDistance->set(config_src.lookup("Pathtracer.FocalDistance"));
		Cfg.Pathtracer.ApertureRadius->set(config_src.lookup("Pathtracer.ApertureRadius"));
		Cfg.Pathtracer.MaxRayDepth = config_src.lookup("Pathtracer.MaxRayDepth");
		Cfg.Pathtracer.RussianRouletteThreshold = config_src.lookup("Pathtracer.RussianRouletteThreshold");
		Cfg.Pathtracer.MinRaysPerPixel->set(config_src.lookup("Pathtracer.MinRaysPerPixel"));

	} catch (const SettingNotFoundException &nfex) {
		ERR("Some setting(s) not found in config.ini");
	} catch (const SettingTypeException &tpex) {
		ERR("Some setting(s) assigned to wrong type");
	}

}

void initialize_asset_config()
{
	Config config_src;
	create_config_src(Cfg.AssetsConfigSource, config_src);

		//---------------- ASSETS -------------------

	try
	{
		// textures
		LOG("---- loading textures (lazy) ----");
		const Setting& textures = config_src.getRoot()["Textures"];
		for (int i=0; i<textures.getLength(); i++) {
			std::string name = textures[i].lookup("Name");
			std::string path = textures[i].lookup("Path");
			int num_channels = textures[i].lookup("Channels");
			int bit_depth = textures[i].lookup("BitDepth");
			int SRGB = textures[i].lookup("SRGB");
			new Texture2D(name, std::string(ROOT_DIR"/") + path, {num_channels, bit_depth, SRGB});
			LOG("'%s': %d channels, %d bpc, SRGB %d", name.c_str(), num_channels, bit_depth, SRGB);
		}

		// materials
		LOG("---- loading materials ----");
		const Setting& materials = config_src.getRoot()["Materials"];
		for (int i=0; i<materials.getLength(); i++) {
			std::string pipeline = materials[i].lookup("Pipeline");
			std::string name = materials[i].lookup("Name");
			const Setting& m = materials[i];
			bool created = true;
			if (pipeline == "DeferredBasepass") {
				std::string albedo = m.exists("Albedo") ? m.lookup("Albedo") : "_white";
				std::string normal = m.exists("Normal") ? m.lookup("Normal") : "_defaultNormal";
				std::string metallic = m.exists("Metallic") ? m.lookup("Metallic") : "_black";
				std::string roughness = m.exists("Roughness") ? m.lookup("Roughness") : "_white";
				std::string ao = m.exists("AO") ? m.lookup("AO") : "_white";
				new DeferredBasepass(name, albedo, normal, metallic, roughness, ao);
			} else {
				WARN("cannot load materials with pipeline '%s' yet. skipping..", pipeline.c_str());
				created = false;
			}
			if (created) LOG("%s", name.c_str());
		}

		// material assignments
		LOG("---- loading material assignments ----");
		const Setting& assignments = config_src.getRoot()["MaterialAssignments"];
		for (int i=0; i<assignments.getLength(); i++) {
			std::string mesh = assignments[i].lookup("Mesh");
			std::string mat_name = assignments[i].lookup("Material");
			Mesh::set_material_name(mesh, mat_name);
			LOG("%s : %s", mesh.c_str(), mat_name.c_str());
		}

	} catch (const SettingNotFoundException &nfex) {
		ERR("Some setting(s) not found in config");
	} catch (const SettingTypeException &tpex) {
		ERR("Some setting(s) assigned to wrong type");
	}

}

void initialize_global_config()
{
	Config config_src;
	create_config_src(ROOT_DIR"/config.ini", config_src);

	try
	{
		Cfg.UseCornellBoxScene = config_src.lookup("UseCornellBoxScene");
		Cfg.SceneSource = std::string(ROOT_DIR"/") + (const char*)config_src.lookup("SceneSource");
		Cfg.AssetsConfigSource = std::string(ROOT_DIR"/") + (const char*)config_src.lookup("AssetsConfigSource");
		Cfg.PathtracerConfigSource = std::string(ROOT_DIR"/") + (const char*)config_src.lookup("PathtracerConfigSource");
		Cfg.RenderDoc = config_src.lookup("RenderDoc");
		Cfg.RTX = config_src.lookup("RTX");

	} catch (const SettingNotFoundException &nfex) {
		ERR("Some setting(s) not found in config.ini");
	} catch (const SettingTypeException &tpex) {
		ERR("Some setting(s) assigned to wrong type");
	}
}

void initialize_all_config()
{
	//-------- read config from config.ini --------
	initialize_global_config();

	//-------- Pathtracer --------
	initialize_pathtracer_config();

	//-------- Assets --------
	initialize_asset_config();
}
