#include "lib.h"

void initialize_config();

template<typename T>
struct CVar {

	// default constructor
	CVar() {}
	// constructor with initialization
	CVar(T _value) : value(_value) {}
	// copy constructor
	CVar(const CVar<T>& cvar) {
		value = cvar.get();
	}

	// getter
	T get() const { return value; }

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
		int UseDOF = 1;
		int MaxRayDepth = 16;
		float RussianRouletteThreshold = 0.05f;
		CVar<int> MinRaysPerPixel = CVar<int>(4);
	}
	Pathtracer;
};

extern ProgramConfig Cfg;
