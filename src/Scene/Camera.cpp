#include "Camera.hpp"
#include "Engine/Program.hpp"
#include <assimp/scene.h>

using namespace glm;

void Camera::set_local_position(vec3 _local_position) {
	local_position_value = _local_position;
}

void Camera::set_rotation(quat _rotation) {
	rotation_value = _rotation;
}

void Camera::set_scale(vec3 _scale) {
	scale_value = vec3(1);
}

Camera::Camera(size_t w, size_t h, bool _ortho) :
		orthographic(_ortho), width(w), height(h) {

	local_position_value = vec3(0);

	// TODO: make these properties
	move_speed = 150.0f;
	rotate_speed = 0.002f;

	fov = radians(60.0f);
	cutoffNear = 0.5f;
	cutoffFar = 40.0f;

	aspect_ratio = (float)w / (float)h;

	locked = false;

	prev_mouse_x = 0;
	prev_mouse_y = 0;

}

// Never really got how camera rotation control works but gonna settle for now...
// Camera is looking down z axis when yaw, pitch, row = 0
// UE4 implementation in: Engine/Source/Runtime/Core/Private/Math/UnrealMath.cpp
void Camera::update_control(float elapsed) {

	if (!locked && !Program::Instance->receiving_text) {
		const Uint8* state = SDL_GetKeyboardState(nullptr);

		vec3 forward = this->forward();
		vec3 right = this->right();

		if (state[SDL_SCANCODE_LSHIFT]) {
			// up, down
			if (state[SDL_SCANCODE_W]) {
				local_position_value.z += move_speed * elapsed;
			}
			if (state[SDL_SCANCODE_S]) {
				local_position_value.z -= move_speed * elapsed;
			}

		} else {
			// WASD movement; E - up; Q - down
			if (state[SDL_SCANCODE_A]) {
				local_position_value -= move_speed * elapsed * right;
			}
			if (state[SDL_SCANCODE_D]) {
				local_position_value += move_speed * elapsed * right;
			}
			if (state[SDL_SCANCODE_S]) {
				local_position_value -= move_speed * elapsed * forward;
			}
			if (state[SDL_SCANCODE_W]) {
				local_position_value += move_speed * elapsed * forward;
			}
			if (state[SDL_SCANCODE_E]) {
				local_position_value.y += move_speed * elapsed;
			}
			if (state[SDL_SCANCODE_Q]) {
				local_position_value.y -= move_speed * elapsed;
			}
		}

		// rotation
		float yaw = glm::yaw(rotation_value);
		float pitch = glm::pitch(rotation_value);
		float roll = glm::roll(rotation_value);

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
		// rotation_value = quatLookAt(normalize(-world_position()), vec3(0, 1, 0));
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

mat4 Camera::camera_to_clip()
{
	mat4 camera_to_clip = orthographic ?
						  ortho(-width/2, width/2, -height/2, height/2, cutoffNear, cutoffFar) :
						  perspective(fov, aspect_ratio, cutoffNear, cutoffFar);
	return camera_to_clip;
}

Camera::Camera(aiCamera* inCamera)
{
	move_speed = 16;
	rotate_speed = 0.002f;

	fov = 2 * inCamera->mHorizontalFOV / inCamera->mAspect;
	cutoffNear = inCamera->mClipPlaneNear;
	cutoffFar = inCamera->mClipPlaneFar;

	width = 0; // only matters for orthographic cameras
	height = 0; // only matters for orthographic cameras
	aspect_ratio = inCamera->mAspect;

	prev_mouse_x = 0;
	prev_mouse_y = 0;
	locked = false;
	orthographic = false;

	auto toVec3 = [](aiVector3D& inVec) {
		return vec3(inVec.x, inVec.y, inVec.z);
	};

	// inherited
	name = inCamera->mName.C_Str();
	local_position_value = toVec3(inCamera->mPosition);

	rotation_value = quatLookAt(toVec3(inCamera->mLookAt), toVec3(inCamera->mUp));
}
