#include "Utils/lib.h"
#include "Utils/myn/CVar.h"
#include <functional>

struct NamedTex;

void initialize_pathtracer_config();
void initialize_basic_asset_config();
void initialize_asset_config();
void initialize_global_config();
void initialize_all_config();

std::vector<NamedTex*> namedtex_list(NamedTex* new_tex = nullptr);

void list_textures();

int find_named_tex(int index);

struct NamedTex {
	NamedTex(std::string _name, uint _tex) : name(_name), tex(_tex) {
		namedtex_list(this);
	}
	std::string name = "";
	uint tex;
	int index;
};

struct ProgramConfig
{
	int UseCornellBoxScene = 0;
	std::string SceneSource = "";
	std::string AssetsConfigSource = "";
	std::string PathtracerConfigSource = "";
	int TestVulkan = 0;
	struct 
	{
		int ISPC = 0;
		CVar<int>* UseBVH = new CVar<int>("UseBVH", 1);
		int SmallWindow = 0;
		int Multithreaded = 1;
		int NumThreads = 8;
		int TileSize = 16;
		int UseDirectLight = 1;
		int AreaLightSamples = 2;
		int UseJitteredSampling = 1;
		CVar<int>* UseDOF = new CVar<int>("UseDOF", 1);
		CVar<float>* FocalDistance = new CVar<float>("FocalDistance", 500);
		CVar<float>* ApertureRadius = new CVar<float>("ApertureRadius", 8);
		int MaxRayDepth = 16;
		float RussianRouletteThreshold = 0.05f;
		CVar<int>* MinRaysPerPixel = new CVar<int>("MinRaysPerPixel", 4);
	}
	Pathtracer;

	CVar<int>* ShowDebugTex = new CVar<int>("ShowDebugTex", 0);
	CVar<int>* DebugTex = new CVar<int>("DebugTex", 0);
	CVar<float>* DebugTexMin = new CVar<float>("DebugTexMin", 0.0f);
	CVar<float>* DebugTexMax = new CVar<float>("DebugTexMax", 1.0f);

	CVar<int>* MaterialSet = new CVar<int>("MaterialSet", 1);

	CVar<float>* Exposure = new CVar<float>("Exposure", 0.0f);
	CVar<int>* Bloom = new CVar<int>("Bloom", 1);
	CVar<float>* BloomThreshold = new CVar<float>("BloomThreshold", 1.0f);
	CVar<int>* ToneMapping = new CVar<int>("Tonemapping", 1);
	CVar<int>* GammaCorrect = new CVar<int>("GammaCorrect", 1);
};

extern ProgramConfig Cfg;
