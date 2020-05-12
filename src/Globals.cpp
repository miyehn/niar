#include "Globals.hpp"

using namespace libconfig;

Config config_src;

ProgramConfig Cfg;

void load_config() {

	try {
		config_src.readFile("config.ini");
	} catch (const FileIOException &fioex) {
		ERR("I/O error while reading config.ini");
		return;
	} catch (const ParseException &pex) {
		ERRF("Config file parse error at %s:%d - %s", pex.getFile(), pex.getLine(), pex.getError());
		return;
	}

	try {
		Cfg.Pathtracer.Multithreaded = config_src.lookup("Pathtracer.Multithreaded");

	} catch (const SettingNotFoundException &nfex) {
		ERR("Some setting(s) not found in config.ini");
	} catch (const SettingTypeException &tpex) {
		ERR("SOme settings(s) assigned to wrong type");
	}
	LOGF("global: cfg %p ptc %p", &Cfg, &Cfg.Pathtracer);

	LOG("***successfully loaded config from file!***");

	Cfg.Pathtracer.MinRaysPerPixel.set(16);
}

