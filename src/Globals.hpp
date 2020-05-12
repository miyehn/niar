#include "lib.h"
#include <mutex>

void load_config();

template<typename T>
struct CVar {

	T get() {
		if (!initialized) WARN("Getting value of an uninitialized cvar");
		return value;
	}

	void set(T _value) {
		initialized = true;
		std::lock_guard<std::mutex> lock(m);
		value = _value;
	}

private:
	bool initialized = false;
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
		CVar<int> MinRaysPerPixel;
		int MaxRayDepth = 16;
		float RussianRouletteThreshold = 0.05f;
	}
	Pathtracer;
};

extern libconfig::Config config_src;
extern ProgramConfig Cfg;
