#include "Utils/myn/CVar.h"
#include "Utils/myn/Log.h"
#include "Asset.h"
#include <functional>
#include <utility>
#include <libconfig/libconfig.h++>

void initialize_pathtracer_config();
void initialize_basic_asset_config();
void initialize_all_config();

class ConfigFile : Asset
{
public:
	explicit ConfigFile(const std::string& path, const std::function<void(const ConfigFile &cfg)>& loadAction = nullptr);

	template<typename T>
	T lookup(const std::string& cfg_path) const {
		try {
			T value = config.lookup(cfg_path);
			return value;
		} catch (const libconfig::SettingNotFoundException &nfex) {
			ERR("Some setting(s) not found in config.ini");
		} catch (const libconfig::SettingTypeException &tpex) {
			ERR("Some setting(s) assigned to wrong type");
		}
		return T();
	}
private:
	libconfig::Config config = {};
};

struct ProgramConfigOld
{
	struct
	{
		int ISPC = 0;
		myn::CVar<int>* UseBVH = new myn::CVar<int>("UseBVH", 1);
		int SmallWindow = 0;
		int Multithreaded = 1;
		int NumThreads = 8;
		int TileSize = 16;
		int UseDirectLight = 1;
		int AreaLightSamples = 2;
		int UseJitteredSampling = 1;
		myn::CVar<int>* UseDOF = new myn::CVar<int>("UseDOF", 1);
		myn::CVar<float>* FocalDistance = new myn::CVar<float>("FocalDistance", 500);
		myn::CVar<float>* ApertureRadius = new myn::CVar<float>("ApertureRadius", 8);
		int MaxRayDepth = 16;
		float RussianRouletteThreshold = 0.05f;
		myn::CVar<int>* MinRaysPerPixel = new myn::CVar<int>("MinRaysPerPixel", 4);
	}
	Pathtracer;
};

extern ProgramConfigOld Cfg;

extern ConfigFile* Config;
