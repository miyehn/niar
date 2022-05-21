#include "Utils/myn/Log.h"
#include "Asset.h"
#include <functional>
#include <utility>
#include <libconfig/libconfig.h++>

class ConfigFile : Asset
{
public:
	explicit ConfigFile(const std::string& relative_path, const std::function<void(const ConfigFile *cfg)>& loadAction = nullptr);

	template<typename T>
	T lookup(const std::string& cfg_path) const {
		try {
			T value = config.lookup(cfg_path);
			return value;
		} catch (const libconfig::SettingNotFoundException &nfex) {
			ERR("\"%s\" not found in config file \"%s\"", cfg_path.c_str(), path.c_str());
		} catch (const libconfig::SettingTypeException &tpex) {
			ERR("\"%s\" is not of type %s", cfg_path.c_str(), typeid(T).name());
		}
		return T();
	}
private:
	libconfig::Config config = {};
};

extern ConfigFile* Config;
