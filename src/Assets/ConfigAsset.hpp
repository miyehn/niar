#include "Utils/myn/Log.h"
#include "Asset.h"
#include <functional>
#include <utility>
#include <libconfig/libconfig.h++>

class ConfigAsset : Asset
{
public:
	explicit ConfigAsset(
		const std::string& relative_path,
		bool allow_reload = true,
		const std::function<void(const ConfigAsset *cfg)>& loadAction = nullptr);

	template<typename T>
	T lookup(const std::string& cfg_path) const {
		try {
			T value = config.lookup(cfg_path);
			return value;
		} catch (const libconfig::SettingNotFoundException &nfex) {
			ERR("\"%s\" not found in config file \"%s\": %s",
				cfg_path.c_str(), relative_path.c_str(), nfex.what())
		} catch (const libconfig::SettingTypeException &tpex) {
			ERR("\"%s\" is not of type %s: %s"
				, cfg_path.c_str(), typeid(T).name(), tpex.what())
		}
		return T();
	}
	const libconfig::Setting& lookupRaw(const std::string& cfg_path) const {
		return config.lookup(cfg_path);
	}
private:
	libconfig::Config config = {};
};

extern ConfigAsset* Config;
