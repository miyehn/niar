#pragma once
#include "Updatable.hpp"

struct Camera : Updatable {

  static Camera* Active;

  Camera(size_t w, size_t h);

  // inherited
  void update(float time_elapsed);
  bool handle_event(SDL_Event event);

  // properties, can be set by the program
  vec3 position = vec3(0, -4, 4);
  float yaw = 0.0f;
  float pitch = radians(45.0f);
  float row = 0.0f;

  float move_speed = 4.0f;
  float rotate_speed = 0.003f;

  float fov = radians(65.0f);
  float cutoffNear = 0.1f;
  float cutoffFar = 100.0f;

  size_t width;
  size_t height;
  float aspect_ratio;

  // functions
  mat4 world_to_clip();

private:
  int prev_mouse_x = 0;
  int prev_mouse_y = 0;

};
