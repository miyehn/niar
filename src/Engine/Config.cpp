#include "Config.hpp"
#include "Render/Texture.h"
#include "Asset.h"

ProgramConfigOld Cfg;

ConfigFile* Config = nullptr;

void create_config_src(const std::string &path, libconfig::Config& out_config_src)
{
	try {
		out_config_src.readFile(path.c_str());
	} catch (const libconfig::FileIOException &fioex) {
		ERR("I/O error while reading %s", path.c_str())
		return;
	} catch (const libconfig::ParseException &pex) {
		ERR("Config file parse error at %s:%d - %s", pex.getFile(), pex.getLine(), pex.getError())
		return;
	}
}

void initialize_pathtracer_config()
{
	libconfig::Config config_src;
	create_config_src(ROOT_DIR"/" + Config->lookup<std::string>("PathtracerConfigSource"), config_src);

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

	} catch (const libconfig::SettingNotFoundException &nfex) {
		ERR("Some setting(s) not found in config.ini");
	} catch (const libconfig::SettingTypeException &tpex) {
		ERR("Some setting(s) assigned to wrong type");
	}

}

void initialize_all_config()
{
	//-------- Pathtracer --------
	initialize_pathtracer_config();
}

ConfigFile::ConfigFile(
	const std::string& path,
	const std::function<void(const ConfigFile &cfg)> &loadAction) : Asset(path, nullptr)
{
	load_action = [this, path, loadAction]() {
		create_config_src(ROOT_DIR"/" + path, config);
		if (loadAction) {
			try {
				loadAction(*this);
			} catch (const libconfig::SettingNotFoundException &nfex) {
				ERR("Some setting(s) not found in config.ini");
			} catch (const libconfig::SettingTypeException &tpex) {
				ERR("Some setting(s) assigned to wrong type");
			}
		}
	};
	reload();
}