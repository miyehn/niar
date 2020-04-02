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

#if 1 // cube
  // load and process mesh
  std::vector<Mesh*> meshes = Mesh::LoadMeshes("../media/cube.fbx");
  Mesh* cube = meshes[0];
  cube->shader.set_parameters = [cube]() {
    mat4 OBJECT_TO_CLIP = Camera::Active->world_to_clip() * cube->object_to_world();
    cube->shader.set_mat4("OBJECT_TO_CLIP", OBJECT_TO_CLIP);
  };
	cube->local_position += vec3(2, 0, 3);
	cube->bsdf = new Diffuse(vec3(1, 0.4, 0.4));
  scene->add_child(static_cast<Drawable*>(cube));

  meshes = Mesh::LoadMeshes("../media/plane.fbx");
  Mesh* plane = meshes[0];
  plane->shader.set_parameters = [plane]() {
    mat4 OBJECT_TO_CLIP = Camera::Active->world_to_clip() * plane->object_to_world();
    plane->shader.set_mat4("OBJECT_TO_CLIP", OBJECT_TO_CLIP);
  };
  //plane->bsdf->emission = vec3(1.0f);
  plane->local_position = vec3(0, 0, 5.5);
  plane->scale = vec3(4, 4, 1);
	plane->bsdf = new Diffuse();
	plane->bsdf->Le = vec3(3); // emissive plane
  scene->add_child(static_cast<Drawable*>(plane));

  Pathtracer::Instance->load_scene(*scene);
#else // grass
  scene->add_child(static_cast<Drawable*>(new Pathtracer(width, height)));
#endif

  scenes.push_back(scene);
}

void Program::release_resources() {
  LOG("release resources");
  delete Camera::Active;
  delete Pathtracer::Instance;
  if (Shader::Basic.id) glDeleteProgram(Shader::Basic.id);
}
