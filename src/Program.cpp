#include "Program.hpp"
#include "Scene.hpp"
#include "GrassField.hpp"
#include "Cube.hpp"
#include "utils.hpp"

void Program::setup() {

  Drawable* grass = new GrassField(6);
  int num = load_meshes("../media/longcube.fbx", grass);

  scenes.push_back(new Scene(grass));
}

