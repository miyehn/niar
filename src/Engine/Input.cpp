#include "Input.hpp"
#include "Utils/Utils.hpp"
#include "Asset/Shader.h"
#include "Asset/Blit.h"
#include "Asset/Texture.h"
#include "Asset/Material.h"
#include "Asset/Mesh.h"
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

		// shaders
		const Setting& shaders = config_src.getRoot()["Shaders"];
		LOG("---- loading shaders ----");
		for (int i=0; i<shaders.getLength(); i++) {
			int type = shaders[i].lookup("Type");
			std::string name = shaders[i].lookup("Name");
			Shader* shader;
			if (type == 1) {
				std::string fs = shaders[i].lookup("FS");
				shader = new Blit(ROOT_DIR"/" + fs);
			} else if (type == 2) {
				std::string vs = shaders[i].lookup("VS");
				std::string fs = shaders[i].lookup("FS");
				shader = new Shader(ROOT_DIR"/" + vs, ROOT_DIR"/" + fs);
			} else if (type == 4) {
				std::string vs = shaders[i].lookup("VS");
				std::string fs = shaders[i].lookup("FS");
				std::string tc = shaders[i].lookup("TC");
				std::string te = shaders[i].lookup("TE");
				shader = new Shader(
					ROOT_DIR"/" + vs,
					ROOT_DIR"/" + fs,
					ROOT_DIR"/" + tc,
					ROOT_DIR"/" + te);
			}
			Shader::add(name, shader);
			shader->name = name;
		}

		// textures
		LOG("---- loading textures (lazy) ----");
		const Setting& textures = config_src.getRoot()["Textures"];
		for (int i=0; i<textures.getLength(); i++) {
			std::string name = textures[i].lookup("Name");
			std::string path = textures[i].lookup("Path");
			int SRGB = textures[i].lookup("SRGB");
			Texture::set_resource_info(name, ROOT_DIR"/" + path, SRGB);
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
				MatDeferredGeometry* mat = new MatDeferredGeometry();
				mat->albedo_map = Texture::get(albedo);
				mat->normal_map = Texture::get(normal);
				mat->metallic_map = Texture::get(metallic);
				mat->roughness_map = Texture::get(roughness);
				mat->ao_map = Texture::get(ao);
				Material::add_to_pool(name, mat);

			} else if (shader == "basic") {
				std::string base_color = m.exists("BaseColor") ? m.lookup("BaseColor") : "white";
				MatBasic* mat = new MatBasic();
				mat->base_color = Texture::get(base_color);
				Material::add_to_pool(name, mat);

			} else if (shader == "geometry_basic") {
				std::string albedo = m.exists("Albedo") ? m.lookup("Albedo") : "white";
				MatDeferredGeometryBasic* mat = new MatDeferredGeometryBasic();
				mat->albedo_map = Texture::get(albedo);
				Material::add_to_pool(name, mat);

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
			std::string mat = assignments[i].lookup("Material");
			Mesh::set_material_name_for(mesh, mat);
			LOG("%s : %s", mesh.c_str(), mat.c_str());
		}

	} catch (const SettingNotFoundException &nfex) {
		ERR("Some setting(s) not found in config.ini");
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
		Cfg.TestVulkan = config_src.lookup("TestVulkan");

		Cfg.ShowDebugTex->set(config_src.lookup("ShowDebugTex"));
		Cfg.DebugTex->set(config_src.lookup("DebugTex"));
		Cfg.DebugTexMin->set(config_src.lookup("DebugTexMin"));
		Cfg.DebugTexMax->set(config_src.lookup("DebugTexMax"));

		Cfg.MaterialSet->set(config_src.lookup("MaterialSet"));

		Cfg.Exposure->set(config_src.lookup("Exposure"));
		Cfg.Bloom->set(config_src.lookup("Bloom"));
		Cfg.BloomThreshold->set(config_src.lookup("BloomThreshold"));
		Cfg.ToneMapping->set(config_src.lookup("ToneMapping"));
		Cfg.GammaCorrect->set(config_src.lookup("GammaCorrect"));

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

std::vector<NamedTex*> namedtex_list(NamedTex* new_tex) {
	static std::vector<NamedTex*> NamedTextures = std::vector<NamedTex*>();
	if (new_tex) {
		new_tex->index = NamedTextures.size();
		NamedTextures.push_back(new_tex);
	}
	return NamedTextures;
}

void list_textures() {
	auto Textures = namedtex_list();
	LOGR("There are %lu named textures:", Textures.size());
	for (int i=0; i<Textures.size(); i++) {
		LOGR("\t%d - %s", Textures[i]->index, Textures[i]->name.c_str());
	}
}

int find_named_tex(int index) {
	auto Textures = namedtex_list();
	for (int i=0; i<Textures.size(); i++) {
		if (index == Textures[i]->index) return Textures[i]->tex;
	}
	return -1;
}
