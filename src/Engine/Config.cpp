#include "Config.hpp"
#include "Render/Texture.h"
#include "Asset.h"

ConfigFile* Config = nullptr;

void create_config_src(const std::string &absolute_path, libconfig::Config& out_config_src)
{
	try {
		out_config_src.readFile(absolute_path.c_str());
	} catch (const libconfig::FileIOException &fioex) {
		ERR("I/O error while reading %s", absolute_path.c_str())
		return;
	} catch (const libconfig::ParseException &pex) {
		ERR("Config file parse error at %s:%d - %s", pex.getFile(), pex.getLine(), pex.getError())
		return;
	}
}

ConfigFile::ConfigFile(
	const std::string& relative_path,
	const std::function<void(const ConfigFile *cfg)> &loadAction) : Asset(relative_path, nullptr)
{
	load_action = [this, relative_path, loadAction]() {
		create_config_src(ROOT_DIR"/" + relative_path, config);
		if (loadAction) {
			try {
				loadAction(this);
			} catch (const libconfig::SettingNotFoundException &nfex) {
				ERR("Some setting(s) not found in global.ini");
			} catch (const libconfig::SettingTypeException &tpex) {
				ERR("Some setting(s) assigned to wrong type");
			}
		}
	};
	reload();
}