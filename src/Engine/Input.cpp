#include "Input.hpp"
#include "Scene/AABB.hpp"
#include "Render/Mesh.h"
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

	try
	{
		//---------------- ASSETS -------------------

		// textures
		LOG("---- loading textures (lazy) ----");
		const Setting& textures = config_src.getRoot()["Textures"];
		for (int i=0; i<textures.getLength(); i++) {
			std::string name = textures[i].lookup("Name");
			std::string path = textures[i].lookup("Path");
			int SRGB = textures[i].lookup("SRGB");
			// GlTexture::set_resource_info(name, ROOT_DIR"/" + path, SRGB);
			LOG("set texture path for '%s', SRGB: %d", name.c_str(), SRGB);
		}

		// materials
		LOG("---- loading materials ----");
		const Setting& materials = config_src.getRoot()["Materials"];
		for (int i=0; i<materials.getLength(); i++) {
			std::string shader = materials[i].lookup("Shader");
			std::string name = materials[i].lookup("Name");
			const Setting& m = materials[i];
			bool created = true;
			if (shader == "geometry") {
				std::string albedo = m.exists("Albedo") ? m.lookup("Albedo") : "white";
				std::string normal = m.exists("Normal") ? m.lookup("Normal") : "default_normal";
				std::string metallic = m.exists("Metallic") ? m.lookup("Metallic") : "black";
				std::string roughness = m.exists("Roughness") ? m.lookup("Roughness") : "white";
				std::string ao = m.exists("AO") ? m.lookup("AO") : "white";
				/*
				MatDeferredGeometry* mat = new MatDeferredGeometry();
				mat->albedo_map = GlTexture::get(albedo);
				mat->normal_map = GlTexture::get(normal);
				mat->metallic_map = GlTexture::get(metallic);
				mat->roughness_map = GlTexture::get(roughness);
				mat->ao_map = GlTexture::get(ao);
				GlMaterial::add_to_pool(name, mat);
				 */

			} else {
				WARN("cannot load materials with shader '%s' yet. skipping..", shader.c_str());
				created = false;
			}
			if (created) LOG("%s", name.c_str());
		}

		// material assignments
		LOG("---- loading material assignments ----");
		const Setting& assignments = config_src.getRoot()["MaterialAssignments"];
		for (int i=0; i<assignments.getLength(); i++) {
			std::string mesh = assignments[i].lookup("Mesh");
			std::string mat = assignments[i].lookup("GlMaterial");
			Mesh::set_material_name_for(mesh, mat);
			LOG("%s : %s", mesh.c_str(), mat.c_str());
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
