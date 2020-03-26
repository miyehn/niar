#include "Program.hpp"
#include "Scene.hpp"
#include "GrassField.hpp"
#include "Cube.hpp"
#include "utils.hpp"

void Program::setup() { 

  //scenes.push_back(new Scene(new GrassField(6)));

  Drawable* cube = new Cube();
  new GrassField(6, cube); // grassfield as child of cube

  scenes.push_back(new Scene(cube));
}

