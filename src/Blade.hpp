#include "lib.h"

using namespace std;
using namespace glm;

struct Blade{
  Blade(vec3 root);

  vec3 root; // v0
  vec3 above; // v1
  vec3 ctrl; // v2
  vec3 up; // a unit vector
  float orientation;
  float height;
  float width;
  float stiffness;

};
