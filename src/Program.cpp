#include "Program.hpp"
#include "Camera.hpp"
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
	Camera::Active->position = vec3(0, 0, 0);
	Camera::Active->cutoffFar = 1000.0f;
	Camera::Active->fov = radians(30.0f);

	Scene* scene = new Scene("my scene");

	// cornell box scene
	std::vector<Mesh*> meshes = Mesh::LoadMeshes(ROOT_DIR"/media/cornell_box.fbx", false);
	for (int i=0; i<meshes.size(); i++) { // 4 is floor
		Mesh* mesh = meshes[i];
		mesh->bsdf = new Diffuse(vec3(0.6f));
		if (i==1) {// right
			mesh->bsdf->albedo = vec3(0.4f, 0.4f, 0.6f); 
		} else if (i==2) {// left
			mesh->bsdf->albedo = vec3(0.6f, 0.4f, 0.4f); 
		}
		scene->add_child(static_cast<Drawable*>(mesh));
	}

	meshes = Mesh::LoadMeshes(ROOT_DIR"/media/cornell_light.fbx", false);
	Mesh* light = meshes[0];
	light->bsdf = new Diffuse();
	light->name = "light";
	light->bsdf->set_emission(vec3(10.0f));
	scene->add_child(static_cast<Drawable*>(light));

#if 1
	// add another item to it
	meshes = Mesh::LoadMeshes(ROOT_DIR"/media/prism.fbx", false);
	Mesh* mesh = meshes[0];
	mesh->bsdf = new Mirror();//new Diffuse(vec3(0.6f));
	mesh->name = "prism";
	scene->add_child(static_cast<Drawable*>(mesh));
#endif

	Scene::Active = scene;

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

	Scene* scene = new Scene("my scene");
	scene->initialize_graphics();

	int w, h;
	w = drawable_width;
	h = drawable_height;

	/* Classic cornell box
	 *
	 * NOTE: it forces everything to be rendered with basic lighting when pathtracer is disabled
	 * bc this scene only contains area(mesh) lights, which is only supported by pathtracer.
	 *
	 * Also this scene itself doesn't contain the two spheres - they're later added in Pathtracer::initialize()
	 * Can alternatively disable the two spheres over there and add other custom meshes (see below)
	 */
	Camera::Active->position = vec3(0, 0, 0);
	Camera::Active->cutoffFar = 1000.0f;
	Camera::Active->fov = radians(30.0f);
	set_cvar("MaterialSet", "0");
	
	// cornell box
	std::vector<Mesh*> meshes = Mesh::LoadMeshes(ROOT_DIR"/media/cornell_box.fbx");
	for (int i=0; i<meshes.size(); i++) { // 4 is floor
		Mesh* mesh = meshes[i];
		mesh->bsdf = new Diffuse(vec3(0.6f));
		if (i==1) {// right
			mesh->bsdf->albedo = vec3(0.4f, 0.4f, 0.6f); 
			//dynamic_cast<MatBasic*>(mesh->materials[0])->tint = vec3(0.4f, 0.4f, 0.6f);
		} else if (i==2) {// left
			mesh->bsdf->albedo = vec3(0.6f, 0.4f, 0.4f); 
			//dynamic_cast<MatBasic*>(mesh->materials[0])->tint = vec3(0.6f, 0.4f, 0.4f);
		}
		scene->add_child(static_cast<Drawable*>(mesh));
	}

	meshes = Mesh::LoadMeshes(ROOT_DIR"/media/cornell_light.fbx");
	Mesh* light = meshes[0];
	light->bsdf = new Diffuse();
	light->name = "light";
	light->bsdf->set_emission(vec3(10.0f));
	scene->add_child(static_cast<Drawable*>(light));

#if 1
	// add another item to it
	meshes = Mesh::LoadMeshes(ROOT_DIR"/media/prism.fbx");
	Mesh* mesh = meshes[0];
	mesh->bsdf = new Mirror();//Diffuse(vec3(0.6f));
	mesh->name = "prism";
	scene->add_child(static_cast<Drawable*>(mesh));
#endif

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
