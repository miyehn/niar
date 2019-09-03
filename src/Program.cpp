#include "Program.hpp"
#include "GrassField.hpp"

using namespace std;
using namespace glm;

void Program::setup() { 

  GameObject* grass_field = new GrassField(&camera, 6);
  objects.push_back(grass_field);

}

