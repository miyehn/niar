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

Pathtracer* Pathtracer::Instance;
Camera* Camera::Active;
Scene* Scene::Active;

void Program::load_resources() {
	
	LOG("loading resources...");
	initialize_config();

	Pathtracer::Instance = new Pathtracer(width, height, "Niar");
	Camera::Active = new Camera(width, height);

}

void Program::setup() {

	Scene* scene = new Scene("my scene");

	int w, h;
	w = drawable_width;
	h = drawable_height;

	/* Load from file or manually setup scene
	 * Renders with specified shader set: defaults to 0 (full deferred)
	 */
	if (!Cfg.UseCornellBoxScene) {

		Camera::Active->move_speed = 16.0f;
		Camera::Active->position = vec3(0, -25, 15);
		Camera::Active->cutoffFar = 100.0f;

		scene->load(Cfg.SceneSource, false);

		// decided it's too much hassle to switch to ofbx for now, so will stay with assimp a little longer.
		// f::import_scene(scene, Cfg.SceneSource.c_str());
	}

	/* Classic cornell box
	 *
	 * NOTE: it forces everything to be rendered with basic lighting when pathtracer is disabled
	 * bc this scene only contains area(mesh) lights, which is only supported by pathtracer.
	 *
	 * Also this scene itself doesn't contain the two spheres - they're later added in Pathtracer::initialize()
	 * Can alternatively disable the two spheres over there and add other custom meshes (see below)
	 */
	else {
		Camera::Active->position = vec3(0, 0, 0);
		Camera::Active->cutoffFar = 1000.0f;
		set_cvar("MaterialSet", "0");
		
		// cornell box
		std::vector<Mesh*> meshes = Mesh::LoadMeshes(ROOT_DIR"/media/cornell_box.fbx");
		for (int i=0; i<meshes.size(); i++) { // 4 is floor
			Mesh* mesh = meshes[i];
			mesh->bsdf = new Diffuse(vec3(0.6f));
			if (i==1) {// right
				mesh->bsdf->albedo = vec3(0.4f, 0.4f, 0.6f); 
				dynamic_cast<MatBasic*>(mesh->materials[0])->tint = vec3(0.4f, 0.4f, 0.6f);
			} else if (i==2) {// left
				mesh->bsdf->albedo = vec3(0.6f, 0.4f, 0.4f); 
				dynamic_cast<MatBasic*>(mesh->materials[0])->tint = vec3(0.6f, 0.4f, 0.4f);
			}
			scene->add_child(static_cast<Drawable*>(mesh));
		}

		meshes = Mesh::LoadMeshes(ROOT_DIR"/media/cornell_light.fbx");
		Mesh* light = meshes[0];
		light->bsdf = new Diffuse();
		light->name = "light";
		light->bsdf->set_emission(vec3(10.0f));
		scene->add_child(static_cast<Drawable*>(light));

#if 0
		// add another item to it
		meshes = Mesh::LoadMeshes(ROOT_DIR"/media/prism.fbx");
		Mesh* mesh = meshes[0];
		mesh->shaders[2].set_parameters = [mesh]() {
			mat4 OBJECT_TO_CLIP = Camera::Active->world_to_clip() * mesh->object_to_world();
			mesh->shaders[2].set_mat4("OBJECT_TO_CLIP", OBJECT_TO_CLIP);
		};
		mesh->bsdf = new Diffuse(vec3(0.6f));
		mesh->bsdf->albedo = vec3(1, 1, 1);
		mesh->name = "prism";
		scene->add_child(static_cast<Drawable*>(mesh));
#endif
	}

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
