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

#if 0 // debug
		for (int i = 0; i < m_config.getRoot().getLength(); i++)
		{
			auto& setting = m_config.getRoot()[i];
			LOG("%s, %i", setting.getName(), setting.getType())
		}
#endif
		try {
			T value;
			if (config.lookupValue(cfg_path.c_str(), value))
				return value;
			else
			ERR("failed to lookup \"%s\" from config file \"%s\"", cfg_path.c_str(), relative_path.c_str())
		} catch (const libconfig::SettingNotFoundException &nfex) {
			ERR("\"%s\" not found in config file \"%s\": %s",
				cfg_path.c_str(), relative_path.c_str(), nfex.what())
		} catch (const libconfig::SettingTypeException &tpex) {
			ERR("\"%s\" is not of type %s: %s"
			, cfg_path.c_str(), typeid(T).name(), tpex.what())
		}
		return T();
	}

	template<typename T, int N>
	void lookupVector(const std::string& cfg_path, T* outValues) const {
		const libconfig::Setting& setting = config.lookup(cfg_path);
		if (setting.isList()) {
			for (int i = 0; i < N; i++) {
				outValues[i] = setting[i];
			}
		} else {
			ERR("value is not a list!")
		}
	}

	const libconfig::Setting& lookupRaw(const std::string& cfg_path) const {
		return config.lookup(cfg_path);
	}
private:
	libconfig::Config config = {};
};

extern ConfigAsset* Config;
