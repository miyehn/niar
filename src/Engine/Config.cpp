#include "Config.hpp"
#include "Render/Mesh.h"
#include "Render/Texture.h"
//#include "Render/Materials/DeferredBasepass.h"
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

void initialize_global_config()
{
	Config config_src;
	create_config_src(ROOT_DIR"/config.ini", config_src);

	try
	{
		Cfg.SceneSource = std::string(ROOT_DIR"/") + (const char*)config_src.lookup("SceneSource");
		Cfg.PathtracerConfigSource = std::string(ROOT_DIR"/") + (const char*)config_src.lookup("PathtracerConfigSource");
		Cfg.RenderDoc = config_src.lookup("Debug.RenderDoc");
		Cfg.RTX = config_src.lookup("Debug.RTX");
		Cfg.CollapseSceneTree = config_src.lookup("Debug.CollapseSceneTree");

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
	// (if in the future I create more config
	//initialize_asset_config();
}
