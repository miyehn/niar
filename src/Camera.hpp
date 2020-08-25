#pragma once
#include "Updatable.hpp"

struct Camera {

  static Camera* Active;

  Camera(size_t w, size_t h, bool _ortho = false, bool use_YPR = true);
	~Camera();

  void update_control(float time_elapsed);

  // properties, can be set by the program
  vec3 position;
	quat rotation;
  float yaw;
  float pitch;
  float roll;

  float move_speed;
  float rotate_speed;

  float fov;
  float cutoffNear;
  float cutoffFar;

	float width, height;
  float aspect_ratio;

	bool orthonormal;
	bool use_YPR;

  // can move & rotate camera?
  void lock();
  void unlock();

  // functions
  mat3 world_to_camera_rotation();
  mat4 world_to_camera();
  mat3 camera_to_world_rotation();
  mat4 camera_to_world();

  mat4 world_to_clip();

  vec3 forward();
  vec3 up();
  vec3 right();

	vec4 ZBufferParams();

private:
  int prev_mouse_x;
  int prev_mouse_y;
  bool locked;
};
