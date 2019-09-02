#pragma once
#include "Updatable.hpp"

// convenience
using namespace glm;

struct Camera : Updatable {

  Camera();

  // inherited
  void update(float time_elapsed);
  bool handle_event(SDL_Event event);

  // properties, can be set by the program
  vec3 position;
  float pitch;
  float fov;
  float cutoffNear = 0.1f;
  float cutoffFar = 30.0f;
  float aspect_ratio;

  // functions
  mat4 obj_to_screen(vec3 obj_location);

};
