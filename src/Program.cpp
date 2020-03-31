#include "Program.hpp"
#include "Camera.hpp"
#include "Scene.hpp"
#include "GrassField.hpp"
#include "Cube.hpp"
#include "Mesh.hpp"
#include "Pathtracer.hpp"
#include "Shader.hpp"

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

#if 1 // torus
  // load and process mesh
  std::vector<Mesh*> meshes = Mesh::LoadMeshes("../media/torus.fbx");
  Mesh* torus = meshes[0];

  torus->shader.set_parameters = [torus]() {
    mat4 OBJECT_TO_CLIP = Camera::Active->world_to_clip() * torus->object_to_world();
    torus->shader.set_mat4("OBJECT_TO_CLIP", OBJECT_TO_CLIP);
  };

  scene->add_child(static_cast<Drawable*>(torus));

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
