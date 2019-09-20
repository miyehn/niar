#include "Program.hpp"
#include "GrassField.hpp"

using namespace std;
using namespace glm;

void Program::setup() { 

  GameObject* field = new GrassField(&camera);
  objects.push_back(field);

}

