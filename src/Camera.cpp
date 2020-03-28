#include "Camera.hpp"
#include "utils.hpp"

Camera::Camera(size_t w, size_t h) {
  width = w;
  height = h;

  position = vec3(0, -4, 2);
  yaw = 0.0f;
  pitch = radians(65.0f);
  roll = 0.0f;

  move_speed = 4.0f;
  rotate_speed = 0.003f;

  fov = radians(60.0f);
  cutoffNear = 0.1f;
  cutoffFar = 100.0f;

  aspect_ratio = (float)w / (float)h;

  int prev_mouse_x = 0;
  int prev_mouse_y = 0;
}

void Camera::update(float elapsed) {

  const Uint8* state = SDL_GetKeyboardState(nullptr);

  vec3 forward = this->forward();
  vec3 right = this->right();

  if (state[SDL_SCANCODE_LSHIFT]) {
    // up, down
    if (state[SDL_SCANCODE_W]) {
      position.z += move_speed * elapsed;
    }
    if (state[SDL_SCANCODE_S]) {
      position.z -= move_speed * elapsed;
    }

    // roll
    if (state[SDL_SCANCODE_A]) {
      roll -= 2 * elapsed;
    }
    if (state[SDL_SCANCODE_D]) {
      roll += 2 * elapsed;
    }

  } else {
    // WASD movement
    if (state[SDL_SCANCODE_A]) {
      position -= move_speed * elapsed * right;
    }
    if (state[SDL_SCANCODE_D]) {
      position += move_speed * elapsed * right;
    }
    if (state[SDL_SCANCODE_S]) {
      position -= move_speed * elapsed * forward;
    }
    if (state[SDL_SCANCODE_W]) {
      position += move_speed * elapsed * forward;
    }
  }

  // rotation
  int mouse_x, mouse_y;
  if (SDL_GetMouseState(&mouse_x, &mouse_y) & SDL_BUTTON_LEFT) {
    float dx = mouse_x - prev_mouse_x;
    float dy = mouse_y - prev_mouse_y;
    yaw -= dx * rotate_speed;
    pitch -= dy * rotate_speed;
  }

  prev_mouse_x = mouse_x;
  prev_mouse_y = mouse_y;
}

bool Camera::handle_event(SDL_Event event) {
  return false;
}

mat4 Camera::world_to_camera_rotation() {
  return rotate(mat4(1), -roll, vec3(0, 0, 1)) *
    rotate(mat4(1), -yaw, vec3(0, 1, 0)) * 
    rotate(mat4(1), -pitch, vec3(1, 0, 0));
}

mat4 Camera::camera_to_world_rotation() {
  return rotate(mat4(1), pitch, vec3(1, 0, 0)) *
    rotate(mat4(1), yaw, vec3(0, 1, 0)) * 
    rotate(mat4(1), roll, vec3(0, 0, 1));
}

mat4 Camera::world_to_clip() {

  mat4 m_world_to_camera = world_to_camera_rotation() * translate(mat4(1), -position);
  mat4 m_camera_to_clip = perspective(fov, aspect_ratio, cutoffNear, cutoffFar);

  return m_camera_to_clip * m_world_to_camera;
}

vec3 Camera::right() {
  return vec3(camera_to_world_rotation() * vec4(1, 0, 0, 1));
}

vec3 Camera::up() {
  return vec3(camera_to_world_rotation() * vec4(0, 1, 0, 1));
}

vec3 Camera::forward() {
  return vec3(camera_to_world_rotation() * vec4(0, 0, -1, 1));
}

