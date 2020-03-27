#pragma once
#include "Updatable.hpp"

struct Camera : Updatable {

  static Camera* Active;

  Camera(size_t w, size_t h);

  // inherited
  void update(float time_elapsed);
  bool handle_event(SDL_Event event);

  // properties, can be set by the program
  glm::vec3 position;
  float pitch;
  float fov;
  float cutoffNear = 0.1f;
  float cutoffFar = 30.0f;

  size_t width;
  size_t height;
  float aspect_ratio;

  // functions
  glm::mat4 world_to_clip();

};
