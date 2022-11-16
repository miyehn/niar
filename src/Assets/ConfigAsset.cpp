#include "ConfigAsset.hpp"
#include "Asset.h"

ConfigAsset* Config = nullptr;

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

ConfigAsset::ConfigAsset(
	const std::string& relative_file_path,
	bool allow_reload,
	const std::function<void(const ConfigAsset *cfg)> &loadAction) : Asset(relative_file_path, nullptr)
{
	reload_condition = [allow_reload](){ return allow_reload; };
	load_action_internal = [this, relative_file_path, loadAction]() {
		create_config_src(ROOT_DIR"/" + relative_file_path, config);
		if (loadAction) {
			try {
				loadAction(this);
			} catch (const libconfig::SettingNotFoundException &nfex) {
				ERR("Some setting(s) not found in '%s'", relative_file_path.c_str());
			} catch (const libconfig::SettingTypeException &tpex) {
				ERR("Some setting(s) in '%s' assigned to wrong type", relative_file_path.c_str());
			}
		}
	};
	reload();
}