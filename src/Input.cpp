#include "Input.hpp"
#include "Utils.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Materials.hpp"
#include "Mesh.hpp"
#include "libconfig/libconfig.h++"

using namespace libconfig;

ProgramConfig Cfg;

void initialize_pathtracer_config() {

	Config config_src;

	try {
		config_src.readFile(ROOT_DIR"/config.ini");
	} catch (const FileIOException &fioex) {
		ERR("I/O error while reading config.ini");
		return;
	} catch (const ParseException &pex) {
		ERRF("Config file parse error at %s:%d - %s", pex.getFile(), pex.getLine(), pex.getError());
		return;
	}

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
		Cfg.Pathtracer.MaxRayDepth = config_src.lookup("Pathtracer.MaxRayDepth");
		Cfg.Pathtracer.RussianRouletteThreshold = config_src.lookup("Pathtracer.RussianRouletteThreshold");
		Cfg.Pathtracer.MinRaysPerPixel->set(config_src.lookup("Pathtracer.MinRaysPerPixel"));

	} catch (const SettingNotFoundException &nfex) {
		ERR("Some setting(s) not found in config.ini");
	} catch (const SettingTypeException &tpex) {
		ERR("Some setting(s) assigned to wrong type");
	}

}

void initialize_config() {

	//-------- read config from config.ini --------

	Config config_src;

	try {
		config_src.readFile(ROOT_DIR"/config.ini");
	} catch (const FileIOException &fioex) {
		ERR("I/O error while reading config.ini");
		return;
	} catch (const ParseException &pex) {
		ERRF("Config file parse error at %s:%d - %s", pex.getFile(), pex.getLine(), pex.getError());
		return;
	}

	try {
		// pathtracer
		Cfg.UseCornellBoxScene = config_src.lookup("UseCornellBoxScene");
		Cfg.SceneSource = std::string(ROOT_DIR"/") + (const char*)config_src.lookup("SceneSource");

		Cfg.Pathtracer.ISPC = config_src.lookup("Pathtracer.ISPC");
		Cfg.Pathtracer.UseBVH->set(config_src.lookup("Pathtracer.UseBVH"));
		Cfg.Pathtracer.SmallWindow = config_src.lookup("Pathtracer.SmallWindow");
		Cfg.Pathtracer.Multithreaded = config_src.lookup("Pathtracer.Multithreaded");
		Cfg.Pathtracer.NumThreads = config_src.lookup("Pathtracer.NumThreads");
		Cfg.Pathtracer.TileSize = config_src.lookup("Pathtracer.TileSize");
		Cfg.Pathtracer.UseDirectLight = config_src.lookup("Pathtracer.UseDirectLight");
		Cfg.Pathtracer.UseJitteredSampling = config_src.lookup("Pathtracer.UseJitteredSampling");
		Cfg.Pathtracer.UseDOF->set(config_src.lookup("Pathtracer.UseDOF"));
		Cfg.Pathtracer.MaxRayDepth = config_src.lookup("Pathtracer.MaxRayDepth");
		Cfg.Pathtracer.RussianRouletteThreshold = config_src.lookup("Pathtracer.RussianRouletteThreshold");
		Cfg.Pathtracer.MinRaysPerPixel->set(config_src.lookup("Pathtracer.MinRaysPerPixel"));

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
			LOGF("set texture path for '%s', SRGB: %d", name.c_str(), SRGB);
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
				WARNF("cannot load materials with shader '%s' yet. skipping..", shader.c_str());
				created = false;
			}
			if (created) LOGF("%s", name.c_str());
		}

		// material assignments
		LOG("---- loading material assignments ----");
		const Setting& assignments = config_src.getRoot()["MaterialAssignments"];
		for (int i=0; i<assignments.getLength(); i++) {
			std::string mesh = assignments[i].lookup("Mesh");
			std::string mat = assignments[i].lookup("Material");
			Mesh::set_material_name_for(mesh, mat);
			LOGF("%s : %s", mesh.c_str(), mat.c_str());
		}

	} catch (const SettingNotFoundException &nfex) {
		ERR("Some setting(s) not found in config.ini");
	} catch (const SettingTypeException &tpex) {
		ERR("Some setting(s) assigned to wrong type");
	}

}

std::vector<CVarBase*> cvars_list(CVarBase* new_cvar) {
	static std::vector<CVarBase*> ConsoleVariables = std::vector<CVarBase*>();
	if (new_cvar) ConsoleVariables.push_back(new_cvar);
	return ConsoleVariables;
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
	LOGFR("There are %d named textures:", Textures.size());
	for (int i=0; i<Textures.size(); i++) {
		LOGFR("\t%d - %s", Textures[i]->index, Textures[i]->name.c_str());
	}
}

int find_named_tex(int index) {
	auto Textures = namedtex_list();
	for (int i=0; i<Textures.size(); i++) {
		if (index == Textures[i]->index) return Textures[i]->tex;
	}
	return -1;
}

CVarBase* find_cvar(std::string name) {
	auto ConsoleVariables = cvars_list();
	for (int i=0; i<ConsoleVariables.size(); i++) {
		if (lower(ConsoleVariables[i]->name) == lower(name)) {
			return ConsoleVariables[i];
		}
	}
	return nullptr;
}

void list_cvars() {
	auto ConsoleVariables = cvars_list();

	LOGFR("There are %d console variables:", ConsoleVariables.size());
	for (int i=0; i<ConsoleVariables.size(); i++) {
		if (CVar<int>* cvar = dynamic_cast<CVar<int>*>(ConsoleVariables[i])) {
			LOGFR("\t%s (int)\t%d", cvar->get_name().c_str(), cvar->get());

		} else if (CVar<float>* cvar = dynamic_cast<CVar<float>*>(ConsoleVariables[i])) {
			LOGFR("\t%s (float)\t%f", cvar->get_name().c_str(), cvar->get());

		}
	}
}

void log_cvar(std::string name){
	if (auto found = find_cvar(name)) {
		if (CVar<int>* cvar = dynamic_cast<CVar<int>*>(found)) {
			LOGFR("int, %s, %d", cvar->get_name().c_str(), cvar->get());
			return;

		} else if (CVar<float>* cvar = dynamic_cast<CVar<float>*>(found)) {
			LOGFR("float, %s, %f", cvar->get_name().c_str(), cvar->get());
			return;
		}
	}
	LOGR("cvar not found.");
}

void set_cvar(std::string name, std::string val) {
	if (auto found = find_cvar(name)) {
		if (CVar<int>* cvar = dynamic_cast<CVar<int>*>(found)) {
			try {
				int n = std::stoi(val);
				cvar->set(n);
			} catch (std::invalid_argument const &e) {
				LOGR("invalid int argument");
			}

		} else if (CVar<float>* cvar = dynamic_cast<CVar<float>*>(found)) {
			try {
				float n = std::stof(val);
				cvar->set(n);
			} catch (std::invalid_argument const &e) {
				LOGR("invalid float argument");
			}

		}
	} else {
		LOGR("cvar not found.");
	}
}
