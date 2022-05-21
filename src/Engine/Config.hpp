#include "Utils/myn/CVar.h"
#include "Asset.h"
#include <functional>
#include <utility>

void initialize_pathtracer_config();
void initialize_basic_asset_config();
void initialize_global_config();
void initialize_all_config();

class ConfigFile : Asset
{
	//ConfigFile(const std::string& path);
};

struct ProgramConfigOld
{
	std::string SceneSource;
	std::string PathtracerConfigSource;
	int RenderDoc;
	int RTX;
	int CollapseSceneTree;
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
