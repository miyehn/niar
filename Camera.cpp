#include "Camera.hpp"

Camera::Camera(size_t w, size_t h) {
  position = vec3(0.0f, -30.0f, 12.0f);
  pitch = radians(45.0f);
  fov = radians(40.0f);
  cutoffNear = 0.1f;
  cutoffFar = 200.0f;
  this->aspect_ratio = (float)w / (float)h;
}

void Camera::update(float time_elapsed) {
}

bool Camera::handle_event(SDL_Event event) {
  switch (event.type) {
    case SDL_KEYDOWN: 
    {
      SDL_Keycode key = event.key.keysym.sym;
      if (key == SDLK_w) {
        position.y += 0.2f;
      } else if (key == SDLK_s) {
        position.y -= 0.2f;
      } else if (key == SDLK_a) {
        position.x -= 0.2f;
      } else if (key == SDLK_d) {
        position.x += 0.2f;
      }
      return true;
    }
    case SDL_MOUSEMOTION:
    {
      if (event.motion.state & SDL_BUTTON_LMASK) {
        float dy = (float)event.motion.yrel;
        pitch -= 0.5f * radians(dy);
      }
      return false;
    }
    default:
      return false;
  }
}

/**
 * model space (input coordinates),
 * world space (model placement in world),
 * camera space (inverse of camera placement in world),
 * multiply by perspective matrix and send to vert
 * tip: could do: transpose(make_mat4(entries)) to get a matrix from 16 entries
 */
mat4 Camera::obj_to_screen() {
    
  // mat4 worldTransform = translate(mat4(1.0f), obj_location); // world space
  mat4 cameraR = glm::rotate(mat4(1.0f), pitch, vec3(1.0f, 0.0f, 0.0f));
  mat4 cameraTransform = glm::translate(mat4(1.0f), position) * cameraR;
  cameraTransform = glm::inverse(cameraTransform); // camera space
  
  mat4 projection = perspective(fov, aspect_ratio, cutoffNear, cutoffFar);
  
  return projection * cameraTransform;// * worldTransform;
}
