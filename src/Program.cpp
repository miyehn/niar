#include "Program.hpp"
#include "Camera.hpp"
#include "Scene.hpp"
#include "GrassField.hpp"
#include "Cube.hpp"
#include "Mesh.hpp"
#include "Pathtracer.hpp"
#include "Shader.hpp"
#include "BSDF.hpp"

Shader Shader::Basic;
Pathtracer* Pathtracer::Instance;
Camera* Camera::Active;

void Program::load_resources() {
  
  LOG("loading resources...");

  Shader::Basic = Shader("../shaders/basic.vert", "../shaders/basic.frag");
  Pathtracer::Instance = new Pathtracer(width, height, "Niar");
  Camera::Active = new Camera(width, height);

}

void Program::setup() {

  Scene* scene = new Scene("my scene");

#if 0 // cube with plane lighting
	Camera::Active->move_speed = 4.0f;
	Camera::Active->position = vec3(0, -6, 4);

  // load and process mesh
  std::vector<Mesh*> meshes = Mesh::LoadMeshes("../media/cube.fbx");
  Mesh* cube = meshes[0];
  cube->shader.set_parameters = [cube]() {
    mat4 OBJECT_TO_CLIP = Camera::Active->world_to_clip() * cube->object_to_world();
    cube->shader.set_mat4("OBJECT_TO_CLIP", OBJECT_TO_CLIP);
  };
	cube->local_position += vec3(2, 0, 3);
	cube->scale = vec3(1.2f);
	cube->bsdf = new Diffuse(vec3(1.0f, 0.4f, 0.4f));
  scene->add_child(static_cast<Drawable*>(cube));

  meshes = Mesh::LoadMeshes("../media/plane.fbx");
  Mesh* plane = meshes[0];
  plane->shader.set_parameters = [plane]() {
    mat4 OBJECT_TO_CLIP = Camera::Active->world_to_clip() * plane->object_to_world();
    plane->shader.set_mat4("OBJECT_TO_CLIP", OBJECT_TO_CLIP);
  };
  //plane->bsdf->emission = vec3(1.0f);
  plane->local_position = vec3(0, 0, 6);
  plane->scale = vec3(4, 4, 1);
	plane->bsdf = new Diffuse();
	plane->bsdf->Le = vec3(1); // emissive plane
  scene->add_child(static_cast<Drawable*>(plane));

  Pathtracer::Instance->load_scene(*scene);

#else // cornell box, centered at (0, 400, 0)
	Camera::Active->position = vec3(0, 0, 0);
	
	// cornell box
	std::vector<Mesh*> meshes = Mesh::LoadMeshes("../media/cornell_box.dae");
	for (int i=0; i<meshes.size(); i++) { // 4 is floor
		Mesh* mesh = meshes[i];
		mesh->shader.set_parameters = [mesh](){
			mat4 OBJECT_TO_CLIP = Camera::Active->world_to_clip() * mesh->object_to_world();
			mesh->shader.set_mat4("OBJECT_TO_CLIP", OBJECT_TO_CLIP);
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
  light->shader.set_parameters = [light]() {
    mat4 OBJECT_TO_CLIP = Camera::Active->world_to_clip() * light->object_to_world();
    light->shader.set_mat4("OBJECT_TO_CLIP", OBJECT_TO_CLIP);
  };
	light->bsdf = new Diffuse();
	light->name = "light";
	light->bsdf->Le = vec3(5);
  scene->add_child(static_cast<Drawable*>(light));

	// add another item to it
	meshes = Mesh::LoadMeshes("../media/blob.dae");
  Mesh* mesh = meshes[0];
  mesh->shader.set_parameters = [mesh]() {
    mat4 OBJECT_TO_CLIP = Camera::Active->world_to_clip() * mesh->object_to_world();
    mesh->shader.set_mat4("OBJECT_TO_CLIP", OBJECT_TO_CLIP);
  };
	mesh->bsdf = new Mirror();
	mesh->bsdf->albedo = vec3(1, 0.5, 0);
	mesh->name = "prism";
  scene->add_child(static_cast<Drawable*>(mesh));

  Pathtracer::Instance->load_scene(*scene);

#endif

  scenes.push_back(scene);
}

void Program::release_resources() {
  LOG("release resources");
  delete Camera::Active;
  delete Pathtracer::Instance;
  if (Shader::Basic.id) glDeleteProgram(Shader::Basic.id);
}
