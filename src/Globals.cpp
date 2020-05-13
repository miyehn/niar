#include "Globals.hpp"
#include "libconfig/libconfig.h++"

using namespace libconfig;

ProgramConfig Cfg;

void initialize_config() {

	//-------- read config from config.ini --------

	Config config_src;

	try {
		config_src.readFile("../config.ini");
	} catch (const FileIOException &fioex) {
		ERR("I/O error while reading config.ini");
		return;
	} catch (const ParseException &pex) {
		ERRF("Config file parse error at %s:%d - %s", pex.getFile(), pex.getLine(), pex.getError());
		return;
	}

	try {
		Cfg.Pathtracer.Debug = config_src.lookup("Pathtracer.Debug");
		Cfg.Pathtracer.SmallWindow = config_src.lookup("Pathtracer.SmallWindow");
		Cfg.Pathtracer.Multithreaded = config_src.lookup("Pathtracer.Multithreaded");
		Cfg.Pathtracer.NumThreads = config_src.lookup("Pathtracer.NumThreads");
		Cfg.Pathtracer.TileSize = config_src.lookup("Pathtracer.TileSize");
		Cfg.Pathtracer.UseDirectLight = config_src.lookup("Pathtracer.UseDirectLight");
		Cfg.Pathtracer.UseJitteredSampling = config_src.lookup("Pathtracer.UseJitteredSampling");
		Cfg.Pathtracer.UseDOF = config_src.lookup("Pathtracer.UseDOF");
		Cfg.Pathtracer.MaxRayDepth = config_src.lookup("Pathtracer.MaxRayDepth");
		Cfg.Pathtracer.RussianRouletteThreshold = config_src.lookup("Pathtracer.RussianRouletteThreshold");
		Cfg.Pathtracer.MinRaysPerPixel->set(config_src.lookup("Pathtracer.MinRaysPerPixel"));

		LOG("*** successfully loaded config from file! ***");

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

void list_cvars() {
	auto ConsoleVariables = cvars_list();

	LOGF("There are %d cvars total atm", ConsoleVariables.size());
	for (int i=0; i<ConsoleVariables.size(); i++) {
		if (CVar<int>* cvar = dynamic_cast<CVar<int>*>(ConsoleVariables[i])) {
			LOGF("int, %s, %d", cvar->get_name().c_str(), cvar->get());

		} else if (CVar<float>* cvar = dynamic_cast<CVar<float>*>(ConsoleVariables[i])) {
			LOGF("float, %s, %f", cvar->get_name().c_str(), cvar->get());

		}
	}
}
