#include "Program.hpp"
#include "Scene.hpp"
#include "GrassField.hpp"
#include "Cube.hpp"
#include "utils.hpp"

void Program::setup() {

  Scene* scene = new Scene("my scene");
  int num = load_meshes("../media/torus.fbx", (Drawable*)scene);
  scenes.push_back(scene);
}

