#include "Camera.hpp"

Camera::Camera() {
  position = vec3(0.0f, -2.0f, 2.0f);
  pitch = radians(45.0f);
  fov = radians(55.0f);
  cutoffNear = 0.1f;
  cutoffFar = 30.0f;
  aspect_ratio = 1.0f;
}

void Camera::update() {
}

bool Camera::handle_event(SDL_Event event) {
  switch (event.type) {
    case SDL_KEYDOWN: {
      SDL_Keycode key = event.key.keysym.sym;
      if (key == SDLK_UP) {
        position.y += 0.1f;
      } else if (key == SDLK_DOWN) {
        position.y -= 0.1f;
      } else if (key == SDLK_LEFT) {
        position.x -= 0.1f;
      } else if (key == SDLK_RIGHT) {
        position.x += 0.1f;
      }
      cout << "handled event." << endl;
      break;
    }
    default:
      break;
  }
  return true;
}

/**
 * model space (input coordinates),
 * world space (model placement in world),
 * camera space (inverse of camera placement in world),
 * multiply by perspective matrix and send to vert
 * tip: could do: transpose(make_mat4(entries)) to get a matrix from 16 entries
 */
mat4 Camera::obj_to_screen(vec3 obj_location) {
    
  mat4 worldTransform = translate(mat4(1.0f), obj_location); // world space
  mat4 cameraR = rotate(mat4(1.0f), pitch, vec3(1.0f, 0.0f, 0.0f));
  mat4 cameraTransform = translate(mat4(1.0f), position) * cameraR;
  cameraTransform = inverse(cameraTransform); // camera space
  
  mat4 projection = perspective(fov, aspect_ratio, cutoffNear, cutoffFar);
  
  return projection * cameraTransform * worldTransform;
}
