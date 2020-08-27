#include "Camera.hpp"
#include "Program.hpp"

Camera::Camera(size_t w, size_t h, bool _ortho, bool _use_YPR) : 
		orthographic(_ortho), use_YPR(_use_YPR), width(w), height(h) {
  position = vec3(0);
  yaw = radians(0.0f);
  pitch = radians(90.0f);
  roll = 0.0f;

	// TODO: make these properties
  move_speed = 150.0f;
  rotate_speed = 0.002f;

  fov = radians(60.0f);
  cutoffNear = 0.5f;
  cutoffFar = 40.0f;

  aspect_ratio = (float)w / (float)h;

  locked = false;

  int prev_mouse_x = 0;
  int prev_mouse_y = 0;

}

void Camera::update_control(float elapsed) {

  if (!locked && !Program::Instance->receiving_text) {
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
      // WASD movement; E - up; Q - down
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
      if (state[SDL_SCANCODE_E]) {
        position.z += move_speed * elapsed;
      }
			if (state[SDL_SCANCODE_Q]) {
				position.z -= move_speed * elapsed;
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
}

Camera::~Camera() {
}

Frustum Camera::frustum() {
	if (orthographic) WARN("creating a frustum for orthographic camera (which is meanlingless)");
	Frustum frus(8);
	vec3 f = forward();
	vec3 u = up();
	vec3 r = right();
	vec3 cnear = position + f * cutoffNear;
	vec3 cfar = position + f * cutoffFar;
	float yraw = std::atan(fov);
	vec3 ynear = u * yraw * cutoffNear;
	vec3 xnear = r * yraw * cutoffNear * aspect_ratio;
	vec3 yfar = u * yraw * cutoffFar;
	vec3 xfar = r * yraw * cutoffFar * aspect_ratio;

	frus[0] = cnear - ynear - xnear;
	frus[1] = cnear - ynear + xnear;
	frus[2] = cnear + ynear - xnear;
	frus[3] = cnear + ynear + xnear;
	frus[4] = cfar - yfar - xfar;
	frus[5] = cfar - yfar + xfar;
	frus[6] = cfar + yfar - xfar;
	frus[7] = cfar + yfar + xfar;
	return frus;
}

// Never really got how camera rotation control works but gonna settle for now...
// Camera is looking down z axis when yaw, pitch, row = 0
// UE4 implementation in: Engine/Source/Runtime/Core/Private/Math/UnrealMath.cpp
mat3 Camera::world_to_camera_rotation() {
	if (use_YPR) {
		mat4 m4 = rotate(mat4(1), -roll, vec3(0, 1, 0)) *
         rotate(mat4(1), -pitch, vec3(1, 0, 0)) * 
         rotate(mat4(1), -yaw, vec3(0, 0, 1));
		return mat3(m4);
	}
	return transpose(camera_to_world_rotation());
}

mat4 Camera::world_to_camera() {
  return mat4(world_to_camera_rotation()) * translate(mat4(1), -position);
}

mat3 Camera::camera_to_world_rotation() {
	mat4 m4;
	if (use_YPR) { 
		m4 = rotate(mat4(1), yaw, vec3(0, 0, 1)) *
         rotate(mat4(1), pitch, vec3(1, 0, 0)) *
         rotate(mat4(1), roll, vec3(0, 1, 0)); }
	else {
		m4 = mat4_cast(rotation);
	}
	return mat3(m4);
}

mat4 Camera::camera_to_world() {
  return translate(mat4(1), position) * mat4(camera_to_world_rotation());
}

mat4 Camera::world_to_clip() {
  mat4 camera_to_clip = orthographic ?
		ortho(-width/2, width/2, -height/2, height/2, cutoffNear, cutoffFar) :
		perspective(fov, aspect_ratio, cutoffNear, cutoffFar);
  return camera_to_clip * world_to_camera();
}

vec3 Camera::right() {
  return camera_to_world_rotation() * vec3(1, 0, 0);
}

vec3 Camera::up() {
  return camera_to_world_rotation() * vec3(0, 1, 0);
}

vec3 Camera::forward() {
  return camera_to_world_rotation() * vec3(0, 0, -1);
}

void Camera::lock() {
  locked = true;
}

void Camera::unlock() {
  locked = false;
}

vec4 Camera::ZBufferParams() {
	vec4 res;
	res.x = cutoffNear;
	res.y = cutoffFar;
	res.z = (1.0f - cutoffFar/cutoffNear) / cutoffFar;
	res.w = (cutoffFar / cutoffNear) / cutoffFar;
	return res;
}
