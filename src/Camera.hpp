#pragma once
#include "Updatable.hpp"

struct Camera : Updatable {

  static Camera* Active;

  Camera(size_t w, size_t h);

  // inherited
  void update(float time_elapsed);
  bool handle_event(SDL_Event event);

  // properties, can be set by the program
  vec3 position;
  float yaw;
  float pitch;
  float roll;

  float move_speed;
  float rotate_speed;

  float fov;
  float cutoffNear;
  float cutoffFar;

  size_t width;
  size_t height;
  float aspect_ratio;

  // functions
  mat4 world_to_camera_rotation();
  mat4 camera_to_world_rotation();
  mat4 world_to_clip();

  vec3 forward();
  vec3 up();
  vec3 right();

private:
  int prev_mouse_x;
  int prev_mouse_y;

};
