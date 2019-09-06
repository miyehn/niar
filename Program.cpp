#include "Program.hpp"
#include "Compute.hpp"

using namespace std;
using namespace glm;

void Program::setup() { 

  GameObject* compute = new Compute(&camera);
  objects.push_back(compute);

}

