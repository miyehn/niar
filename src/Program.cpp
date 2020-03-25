#include "Program.hpp"
#include "GrassField.hpp"

void Program::setup() { 

  Drawable* grass_field = new GrassField(&camera, 6);
  objects.push_back(grass_field);

}

