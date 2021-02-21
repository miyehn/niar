#include "Camera.hpp"
#include "Program.hpp"
#include "Scene.hpp"
#include "GrassField.hpp"
#include "Mesh.hpp"
#include "Pathtracer/Pathtracer.hpp"
#include "Shader.hpp"
#include "Pathtracer/BSDF.hpp"
#include "Pathtracer/Primitive.hpp"
#include "Input.hpp"
#include "Light.hpp"
#include "Texture.hpp"
#include "Materials.hpp"

#include <sys/time.h>
#define TIMER_BEGIN() \
	struct timeval __start_time, __end_time; \
	gettimeofday(&__start_time, 0);

#define TIMER_END() \
	gettimeofday(&__end_time, 0); \
	long __sec = __end_time.tv_sec - __start_time.tv_sec; \
	long __msec = __end_time.tv_usec - __start_time.tv_usec; \
	double execution_time = __sec + __msec * 1e-6;

Pathtracer* Pathtracer::Instance;
Camera* Camera::Active;
Scene* Scene::Active;

void Program::pathtrace_to_file(size_t w, size_t h, const std::string& path) {

	initialize_pathtracer_config();

	Pathtracer::Instance = new Pathtracer(w, h, "command line pathtracer", false);

	Camera::Active = new Camera(w, h);

	Scene::Active = Pathtracer::load_cornellbox_scene();

	Pathtracer::Instance->initialize();
	TRACE("starting..");

	if (Cfg.Pathtracer.ISPC) {
		TRACE("---- ISPC ON ----");
	} else {
		TRACE("---- ISPC OFF ----");
	}

	TIMER_BEGIN();
	Pathtracer::Instance->raytrace_scene_to_buf();
	TIMER_END();
	TRACEF("done! took %f seconds", execution_time);
	Pathtracer::Instance->output_file(path);

	// delete Scene::Active; // TODO: pull out graphics tear down from Scene::~Scene()
	delete Camera::Active;
	delete Pathtracer::Instance;

	return;
}

void Program::load_resources() {
	
	LOG("loading resources...");
	initialize_config();

	Pathtracer::Instance = new Pathtracer(width, height, "Niar");
	Camera::Active = new Camera(width, height);

}

void Program::setup() {

	Scene* scene = Pathtracer::load_cornellbox_scene(true);//new Scene("my scene");
	scene->initialize_graphics();

	set_cvar("MaterialSet", "0");

	Scene::Active = scene;
	scenes.push_back(scene);
}

void Program::release_resources() {
	LOG("releasing resources..");
	for (uint i=0; i<scenes.size(); i++) {
		delete scenes[i];
	}
	delete Camera::Active;
	delete Pathtracer::Instance;
	Texture::cleanup();
	Shader::cleanup();
	Material::cleanup();
}

void Program::process_input() {
	// split input string into tokens
	std::vector<std::string> tokens;
	char buf[128];
	strncpy(buf, input_str.c_str(), 127);
	char* token = strtok(buf, " ");
	while (token) {
		tokens.push_back(std::string(token));
		token = strtok(nullptr, " ");
	}

	uint len = tokens.size();
	if (len==0) {
		LOGR("(invalid input, ignored..)");
		return;
	}
	if (tokens[0] == "ls") {
		if (len==1) list_cvars();
		else if (len==2) {
			if (tokens[1]=="textures" || tokens[1]=="t") list_textures();
			else log_cvar(tokens[1]);
		}
	}
	else if ((tokens[0] == "set" || tokens[0] == "s") && len == 3) {
		set_cvar(tokens[1], tokens[2]);
	}

	else {
		LOGR("(invalid input, ignored..)");
	}
}
