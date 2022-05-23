#include "Camera.hpp"
#include "Utils/myn/Log.h"
#include <tinygltf/tiny_gltf.h>
#if GRAPHICS_DISPLAY
#include <imgui.h>
#endif

using namespace glm;

void Camera::set_local_position(vec3 local_position) {
	_local_position = local_position;
}

void Camera::set_rotation(quat rotation) {
	_rotation = rotation;
}

void Camera::set_scale(vec3 scale) {
	_scale = vec3(1);
}

Camera::Camera(uint32_t w, uint32_t h, bool _ortho) :
		orthographic(_ortho), width(w), height(h) {

	_local_position = vec3(0);
	_rotation = glm::quat(1, 0, 0, 0);

#if GRAPHICS_DISPLAY
	// TODO: make these properties
	move_speed = 150.0f;
	rotate_speed = 0.002f;
#endif

	fov = radians(60.0f);
	cutoffNear = 0.5f;
	cutoffFar = 40.0f;

	aspect_ratio = (float)w / (float)h;

	locked = false;

	prev_mouse_x = 0;
	prev_mouse_y = 0;

}

#if GRAPHICS_DISPLAY
// Never really got how camera rotation control works but gonna settle for now...
// Camera is looking down z axis when yaw, pitch, row = 0
// UE4 implementation in: Engine/Source/Runtime/Core/Private/Math/UnrealMath.cpp
void Camera::update_control(float elapsed) {

	if (!locked) {
		const Uint8* state = SDL_GetKeyboardState(nullptr);

		vec3 forward = this->forward();
		vec3 right = this->right();

		if (state[SDL_SCANCODE_LSHIFT]) {
			// up, down
			if (state[SDL_SCANCODE_W]) {
				_local_position.z += move_speed * elapsed;
			}
			if (state[SDL_SCANCODE_S]) {
				_local_position.z -= move_speed * elapsed;
			}

		} else {
			// WASD movement; E - up; Q - down
			if (state[SDL_SCANCODE_A]) {
				_local_position -= move_speed * elapsed * right;
			}
			if (state[SDL_SCANCODE_D]) {
				_local_position += move_speed * elapsed * right;
			}
			if (state[SDL_SCANCODE_S]) {
				_local_position -= move_speed * elapsed * forward;
			}
			if (state[SDL_SCANCODE_W]) {
				_local_position += move_speed * elapsed * forward;
			}
			if (state[SDL_SCANCODE_E]) {
				_local_position.y += move_speed * elapsed;
			}
			if (state[SDL_SCANCODE_Q]) {
				_local_position.y -= move_speed * elapsed;
			}
		}

		// rotation
		float yaw = glm::yaw(_rotation);
		float pitch = glm::pitch(_rotation);
		float roll = glm::roll(_rotation);

		int mouse_x, mouse_y;
		if (SDL_GetMouseState(&mouse_x, &mouse_y) & SDL_BUTTON_LEFT) {
			float dx = (float)mouse_x - prev_mouse_x;
			float dy = (float)mouse_y - prev_mouse_y;
			yaw += -dx * rotate_speed;
			pitch += -dy * rotate_speed;
		}
		prev_mouse_x = mouse_x;
		prev_mouse_y = mouse_y;

		set_rotation(vec3(pitch, yaw, roll));
	}
	else
	{
		// HACK
		// _rotation = quatLookAt(normalize(-world_position()), vec3(0, 1, 0));
	}
}

void Camera::lock() {
	locked = true;
}

void Camera::unlock() {
	locked = false;
}
#endif

Frustum Camera::frustum() {
	if (orthographic) WARN("creating a frustum for orthographic camera (which is meanlingless)");
	Frustum frus(8);
	vec3 f = forward();
	vec3 u = up();
	vec3 r = right();
	vec3 cnear = world_position() + f * cutoffNear;
	vec3 cfar = world_position() + f * cutoffFar;
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

mat4 Camera::world_to_clip() {
	return camera_to_clip() * world_to_object();
}

vec3 Camera::right() {
	return glm::normalize(glm::mat3(object_to_world()) * vec3(1, 0, 0));
}

vec3 Camera::up() {
	return glm::normalize(glm::mat3(object_to_world()) * vec3(0, 1, 0));
}

vec3 Camera::forward() {
	return glm::normalize(glm::mat3(object_to_world()) * vec3(0, 0, -1));
}

vec4 Camera::ZBufferParams() {
	vec4 res;
	res.x = cutoffNear;
	res.y = cutoffFar;
	res.z = (1.0f - cutoffFar/cutoffNear) / cutoffFar;
	res.w = (cutoffFar / cutoffNear) / cutoffFar;
	return res;
}

mat4 Camera::camera_to_clip()
{
	mat4 camera_to_clip = orthographic ?
						  ortho(-width/2, width/2, -height/2, height/2, cutoffNear, cutoffFar) :
						  perspective(fov, aspect_ratio, cutoffNear, cutoffFar);
	return camera_to_clip;
}

Camera::Camera(const std::string& node_name, const tinygltf::Camera *in_camera) : Camera(0, 0, false)
{
	name = node_name + " | " + in_camera->name;
	LOG("loading camera '%s'..", name.c_str())
#if GRAPHICS_DISPLAY
	move_speed = 5;
	rotate_speed = 0.002f;
#endif
	if (in_camera->type != "perspective")
	{
		WARN("Unsupported camera (%s : %s)", in_camera->name.c_str(), in_camera->type.c_str())
		return;
	}

	const tinygltf::PerspectiveCamera& persp = in_camera->perspective;
	fov = persp.yfov; // in radians
	cutoffNear = persp.znear;
	cutoffFar = persp.zfar;

	width = 0; // ortho only
	height = 0; // ortho only
	aspect_ratio = persp.aspectRatio;

	prev_mouse_x = 0;
	prev_mouse_y = 0;
	locked = false;
	orthographic = false;
}

#if GRAPHICS_DISPLAY
void Camera::draw_config_ui()
{
	if (ImGui::Button("look at origin"))
	{
		glm::quat rot = glm::quatLookAt(-glm::normalize(local_position()), glm::vec3(0, 1, 0));
		set_rotation(rot);
	}
}
#endif