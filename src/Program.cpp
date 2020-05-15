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
	Shader::DepthOnly = Shader("../shaders/position_only.vert", "../shaders/empty.frag");
	Shader::DepthOnly.name = "depth only";
	Blit::CopyDebug = new Blit("../shaders/blit_debug.frag");
  Pathtracer::Instance = new Pathtracer(width, height, "Niar");
  Camera::Active = new Camera(width, height);

}

void Program::setup() {

  Scene* scene = new Scene("my scene");

#if 1 // cube with plane
	Camera::Active->move_speed = 4.0f;
	Camera::Active->position = vec3(0, -10, 1);

  // load and process mesh
  std::vector<Mesh*> meshes = Mesh::LoadMeshes("../media/cube.fbx");
  Mesh* cube = meshes[0];
  cube->shaders[0].set_parameters = [cube]() {
		cube->shaders[0].set_mat3("OBJECT_TO_WORLD_ROT", cube->object_to_world_rotation());
		mat4 o2w = cube->object_to_world();
    cube->shaders[0].set_mat4("OBJECT_TO_WORLD", o2w);
		cube->shaders[0].set_mat4("OBJECT_TO_CLIP", Camera::Active->world_to_clip() * o2w);
  };
	cube->shaders[1].set_parameters = [cube]() {
		mat4 o2w = cube->object_to_world();
		cube->shaders[1].set_mat4("OBJECT_TO_CLIP", Camera::Active->world_to_clip() * o2w);
	};
	cube->shaders[2].set_parameters = [cube]() {
		cube->shaders[2].set_mat3("OBJECT_TO_CAM_ROT", 
				cube->object_to_world_rotation() * Camera::Active->world_to_camera_rotation());
		mat4 o2w = cube->object_to_world();
		cube->shaders[2].set_mat4("OBJECT_TO_CLIP", Camera::Active->world_to_clip() * o2w);
	};
	cube->local_position += vec3(1.5f, 0, 0);
	cube->scale = vec3(1, 8, 4);
	cube->bsdf = new Diffuse(vec3(1.0f, 0.4f, 0.4f));
  scene->add_child(static_cast<Drawable*>(cube));


  meshes = Mesh::LoadMeshes("../media/plane.fbx");
  Mesh* plane = meshes[0];
  plane->shaders[0].set_parameters = [plane]() {
		plane->shaders[0].set_mat3("OBJECT_TO_WORLD_ROT", plane->object_to_world_rotation());
		mat4 o2w = plane->object_to_world();
    plane->shaders[0].set_mat4("OBJECT_TO_WORLD", o2w);
		plane->shaders[0].set_mat4("OBJECT_TO_CLIP", Camera::Active->world_to_clip() * o2w);
  };
	plane->shaders[1].set_parameters = [plane]() {
		mat4 o2w = plane->object_to_world();
		plane->shaders[1].set_mat4("OBJECT_TO_CLIP", Camera::Active->world_to_clip() * o2w);
	};
	plane->shaders[2].set_parameters = [plane]() {
		plane->shaders[2].set_mat3("OBJECT_TO_CAM_ROT", 
				plane->object_to_world_rotation() * Camera::Active->world_to_camera_rotation());
		mat4 o2w = plane->object_to_world();
		plane->shaders[2].set_mat4("OBJECT_TO_CLIP", Camera::Active->world_to_clip() * o2w);
	};
  plane->local_position = vec3(0, 0, 0);
  plane->scale = vec3(8, 8, 1);
	plane->bsdf = new Diffuse();
  scene->add_child(static_cast<Drawable*>(plane));


	// create light(s)
	Light* light = new DirectionalLight(vec3(0.7f, 0.8f, 0.9f), 0.2f, normalize(vec3(0.2, 0.4, -1)));
	scene->add_child(static_cast<Drawable*>(light));
	scene->lights.push_back(light);

	light = new PointLight(vec3(1.0f, 0.8f, 0.5f), 2.0f, vec3(-1, 0, 2));
	scene->add_child(static_cast<Drawable*>(light));
	scene->lights.push_back(light);


#else // cornell box, centered at (0, 400, 0)
	Camera::Active->position = vec3(0, 0, 0);
	set_cvar("ShaderSet", "2");
	
	// cornell box
	std::vector<Mesh*> meshes = Mesh::LoadMeshes("../media/cornell_box.dae");
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

  meshes = Mesh::LoadMeshes("../media/cornell_light.dae");
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
	meshes = Mesh::LoadMeshes("../media/prism.dae");
  Mesh* mesh = meshes[0];
  mesh->shader.set_parameters = [mesh]() {
    mat4 OBJECT_TO_CLIP = Camera::Active->world_to_clip() * mesh->object_to_world();
    mesh->shader.set_mat4("OBJECT_TO_CLIP", OBJECT_TO_CLIP);
  };
	mesh->bsdf = new Diffuse(vec3(0.6f));
	mesh->bsdf->albedo = vec3(1, 1, 1);
	mesh->name = "prism";
  scene->add_child(static_cast<Drawable*>(mesh));
#endif

#endif

	Scene::Active = scene;
  scenes.push_back(scene);
}

void Program::release_resources() {
  LOG("release resources");
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
	if (len==0) return;
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
