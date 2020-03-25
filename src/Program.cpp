#include "Program.hpp"
#include "GrassField.hpp"
#include "Cube.hpp"

void Program::setup() { 

  Drawable* grass_field = new GrassField(&camera, 6);
  objects.push_back(grass_field);

  objects.push_back(new Cube(&camera));
}

