#include "lib.h"
#include <functional>

struct CVarBase;

void initialize_config();

std::vector<CVarBase*> cvars_list(CVarBase* new_cvar = nullptr);

void list_cvars();

struct CVarBase {
	virtual void register_self() { cvars_list(this); }
};

template<typename T>
struct CVar : CVarBase {

	// default constructor
	CVar() { register_self(); }
	// constructor with initialization
	CVar(std::string _name, T _value) : name(_name), value(_value) { 
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
	std::string name = "";
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
		int UseDOF = 1;
		int MaxRayDepth = 16;
		float RussianRouletteThreshold = 0.05f;
		CVar<int>* MinRaysPerPixel = new CVar<int>("MinRaysPerPixel", 4);
	}
	Pathtracer;
};

extern ProgramConfig Cfg;
