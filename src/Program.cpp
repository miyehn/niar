#include "Program.hpp"
#include "Scene.hpp"
#include "GrassField.hpp"
#include "Cube.hpp"
#include "Mesh.hpp"
#include "utils.hpp"

void Program::setup() {

  Scene* scene = new Scene("my scene");

  std::vector<Mesh*> meshes = load_meshes("../media/torus.fbx");
  Mesh* torus = meshes[0];
  assert(torus);
  scene->add_child((Drawable*)torus);

#if 0
  meshes = load_meshes("../media/longcube.fbx");
  Mesh* cube = meshes[0];
  assert(cube);
  cube->local_position += vec3(0, 0, 4);
  torus->add_child((Drawable*) cube);
#endif

  scenes.push_back(scene);
}

