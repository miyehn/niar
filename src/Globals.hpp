#include "lib.h"
#include <functional>

struct CVarBase;
struct NamedTex;

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
	struct 
	{
		int Debug = 0;
		int SmallWindow = 0;
		int Multithreaded = 1;
		int NumThreads = 8;
		int TileSize = 16;
		int UseDirectLight = 1;
		int AreaLightSamples = 2;
		int UseJitteredSampling = 1;
		CVar<int>* UseDOF = new CVar<int>("UseDOF", 1);
		int MaxRayDepth = 16;
		float RussianRouletteThreshold = 0.05f;
		CVar<int>* MinRaysPerPixel = new CVar<int>("MinRaysPerPixel", 4);
	}
	Pathtracer;
};

extern ProgramConfig Cfg;
