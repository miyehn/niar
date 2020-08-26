#include "Program.hpp"
#include "Camera.hpp"
#include "Scene.hpp"
#include "GrassField.hpp"
#include "Cube.hpp"
#include "Mesh.hpp"
#include "Pathtracer.hpp"
#include "Shader.hpp"
#include "BSDF.hpp"
#include "Primitive.hpp"
#include "Globals.hpp"
#include "Light.hpp"

Shader Shader::Basic;
Shader Shader::DeferredBasePass;
Shader Shader::DepthOnly;
Shader Shader::ShadowPassDirectional;
Shader Shader::ShadowPassPoint;
Shader Shader::Distance;

Blit* Blit::CopyDebug;
Pathtracer* Pathtracer::Instance;
Camera* Camera::Active;
Scene* Scene::Active;

void Program::load_resources() {
  
  LOG("loading resources...");
	initialize_config();

  Shader::Basic = Shader("../shaders/basic.vert", "../shaders/basic.frag");
	Shader::Basic.name = "basic";
	Shader::DeferredBasePass = Shader("../shaders/geometry.vert", "../shaders/geometry.frag");
	Shader::DeferredBasePass.name = "deferred";
	Shader::DepthOnly = Shader("../shaders/clip_position.vert", "../shaders/empty.frag");
	Shader::DepthOnly.name = "depth only";
	Shader::ShadowPassDirectional = Shader("../shaders/shadow_pass_directional.vert", "../shaders/shadow_pass_directional.frag");
	Shader::ShadowPassDirectional.name = "shadow pass directional";
	Shader::ShadowPassPoint = Shader("../shaders/shadow_pass_point.vert", "../shaders/shadow_pass_point.frag");
	Shader::ShadowPassPoint.name = "shadow pass point";
	Shader::Distance = Shader("../shaders/world_clip_position.vert", "../shaders/distance.frag");
	Shader::Distance.name = "distance (for point light shadow map)";
	Blit::CopyDebug = new Blit("../shaders/blit_debug.frag");
  Pathtracer::Instance = new Pathtracer(width, height, "Niar");
  Camera::Active = new Camera(width, height);

}

void Program::setup() {

  Scene* scene = new Scene("my scene");

	int w, h;
	w = drawable_width;
	h = drawable_height;

	/* Manually setup scene
	 * Renders with specified shader set: defaults to 0 (deferred)
	 *
	 * TODO: organize this mess... load from file maybe
	 */
	if (!Cfg.UseCornellBoxScene) {

		Camera::Active->move_speed = 6.0f;
		Camera::Active->position = vec3(0, -10, 1);
		Camera::Active->cutoffFar = 100.0f;

		scene->load("../media/with_light.fbx", false);
		
#if 0
		// create light(s)
		DirectionalLight* d_light;
		PointLight* p_light;

		#if 1
		// cool directional light
		d_light = new DirectionalLight(vec3(0.7f, 0.8f, 0.9f), 0.2f, normalize(vec3(0.2, 0.4, -1)));
		//d_light->set_cast_shadow(true);
		scene->add_child(static_cast<Drawable*>(d_light));
		scene->d_lights.push_back(d_light);

		// warm directional light
		d_light = new DirectionalLight(vec3(0.9, 0.8, 0.7), 0.8f, normalize(vec3(-1.5f, 0.6f, -1.0f)));
		//d_light->set_cast_shadow(true);
		scene->add_child(static_cast<Drawable*>(d_light));
		scene->d_lights.push_back(d_light);
		#endif

		#if 1
		// point light
		p_light = new PointLight(vec3(1.0f, 0.8f, 0.5f), 2.0f, vec3(0, -2, 2));
		p_light->set_cast_shadow(true);
		scene->add_child(static_cast<Drawable*>(p_light));
		scene->p_lights.push_back(p_light);
		#endif
#endif

#if 0
		// load and process mesh
		std::vector<Mesh*> meshes = Mesh::LoadMeshes("../media/cube.fbx");
		Mesh* cube = meshes[0];
		cube->local_position += vec3(1.5f, 0, 0);
		cube->scale = vec3(1, 4, 4);
		cube->bsdf = new Diffuse(vec3(1.0f, 0.4f, 0.4f));
		scene->add_child(static_cast<Drawable*>(cube));

		meshes = Mesh::LoadMeshes("../media/plane.fbx");
		Mesh* plane = meshes[0];
		plane->local_position = vec3(0, 0, 0);
		plane->scale = vec3(8, 8, 1);
		plane->bsdf = new Diffuse();
		scene->add_child(static_cast<Drawable*>(plane));
#endif
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
		set_cvar("ShaderSet", "2");
		
		// cornell box
		std::vector<Mesh*> meshes = Mesh::LoadMeshes("../media/cornell_box.fbx");
		for (int i=0; i<meshes.size(); i++) { // 4 is floor
			Mesh* mesh = meshes[i];
			mesh->shaders[2].set_parameters = [mesh](){
				mat3 OBJECT_TO_CAM_ROT = mesh->object_to_world_rotation() * Camera::Active->world_to_camera_rotation();
				mesh->shaders[2].set_mat3("OBJECT_TO_CAM_ROT", OBJECT_TO_CAM_ROT);
				mat4 OBJECT_TO_CLIP = Camera::Active->world_to_clip() * mesh->object_to_world();
				mesh->shaders[2].set_mat4("OBJECT_TO_CLIP", OBJECT_TO_CLIP);
			};
			mesh->bsdf = new Diffuse(vec3(0.6f));
			if (i==1) {// right
				mesh->bsdf->albedo = vec3(0.4f, 0.4f, 0.6f); 
			} else if (i==2) {// left
				mesh->bsdf->albedo = vec3(0.6f, 0.4f, 0.4f); 
			}
			scene->add_child(static_cast<Drawable*>(mesh));
		}

		meshes = Mesh::LoadMeshes("../media/cornell_light.fbx");
		Mesh* light = meshes[0];
		light->shaders[2].set_parameters = [light]() {
			mat3 OBJECT_TO_CAM_ROT = light->object_to_world_rotation() * Camera::Active->world_to_camera_rotation();
			light->shaders[2].set_mat3("OBJECT_TO_CAM_ROT", OBJECT_TO_CAM_ROT);
			mat4 OBJECT_TO_CLIP = Camera::Active->world_to_clip() * light->object_to_world();
			light->shaders[2].set_mat4("OBJECT_TO_CLIP", OBJECT_TO_CLIP);
		};
		light->bsdf = new Diffuse();
		light->name = "light";
		light->bsdf->set_emission(vec3(10.0f));
		scene->add_child(static_cast<Drawable*>(light));

#if 0
		// add another item to it
		meshes = Mesh::LoadMeshes("../media/prism.fbx");
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
  if (Shader::Basic.id) glDeleteProgram(Shader::Basic.id);
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
			if (tokens[1]=="textures") list_textures();
			else log_cvar(tokens[1]);
		}
	}
	else if (tokens[0] == "set" && len == 3) {
		set_cvar(tokens[1], tokens[2]);
	}

	else {
		LOGR("(invalid input, ignored..)");
	}
}
