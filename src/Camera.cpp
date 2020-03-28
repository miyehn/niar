#include "Camera.hpp"

Camera::Camera(size_t w, size_t h) {
  width = w;
  height = h;
  aspect_ratio = (float)w / (float)h;
}

void Camera::update(float elapsed) {

  const Uint8* state = SDL_GetKeyboardState(nullptr);

  // movement
  if (state[SDL_SCANCODE_A]) {
    position.x -= move_speed * elapsed;
  }
  if (state[SDL_SCANCODE_D]) {
    position.x += move_speed * elapsed;
  }
  if (state[SDL_SCANCODE_S]) {
    position.y -= move_speed * elapsed;
  }
  if (state[SDL_SCANCODE_W]) {
    position.y += move_speed * elapsed;
  }
  if (state[SDL_SCANCODE_SPACE]) {
    position.z += move_speed * elapsed;
  }
  if (state[SDL_SCANCODE_Z]) {
    position.z -= move_speed * elapsed;
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

mat4 Camera::world_to_clip() {

  mat4 world_to_camera = 
    rotate(mat4(1), -pitch, vec3(1, 0, 0)) *
    rotate(mat4(1), -yaw, vec3(0, 0, 1)) * 
    translate(mat4(1), -position);
    
  mat4 camera_to_clip = perspective(fov, aspect_ratio, cutoffNear, cutoffFar);

  return camera_to_clip * world_to_camera;
}
