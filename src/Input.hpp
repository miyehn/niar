#include "lib.h"
#include <functional>

struct CVarBase;
struct NamedTex;

void initialize_pathtracer_config();
void initialize_config();

std::vector<NamedTex*> namedtex_list(NamedTex* new_tex = nullptr);

std::vector<CVarBase*> cvars_list(CVarBase* new_cvar = nullptr);

void list_cvars();

void list_textures();

int find_named_tex(int index);

void log_cvar(std::string name);

void set_cvar(std::string name, std::string val);

struct NamedTex {
	NamedTex(std::string _name, uint _tex) : name(_name), tex(_tex) {
		namedtex_list(this);
	}
	std::string name = "";
	uint tex;
	int index;
};

struct CVarBase {
	virtual void register_self() { cvars_list(this); }
	std::string name = "";
};

template<typename T>
struct CVar : public CVarBase {

	// default constructor
	CVar() { register_self(); }
	// constructor with initialization
	CVar(std::string _name, T _value) : value(_value) {
		name = _name;
		register_self(); 
	}

	// getter
	T get() const { return value; }
	std::string get_name() const { return name; }

	// setter
	void set(T _value) {
		std::lock_guard<std::mutex> lock(m);
		value = _value;
	}

private:
	T value;
	std::mutex m;
};

struct ProgramConfig
{
	int UseCornellBoxScene = 0;
	std::string SceneSource = "";
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
