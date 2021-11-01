#pragma once
#include "Engine/Drawable.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

typedef std::vector<glm::vec3> Frustum;

class aiCamera;

struct Camera : Drawable {

	static Camera* Active;

	Camera(size_t w, size_t h, bool _ortho = false, bool use_YPR = true);
	explicit Camera(aiCamera* inCamera);
	~Camera() override;

	void update_control(float time_elapsed);

	// properties, can be set by the program
	//glm::vec3 position;
	//glm::quat rotation;
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

	bool orthographic;
	bool use_YPR;

	Frustum frustum();

	// can move & rotate camera?
	void lock();
	void unlock();

	// functions

	void set_local_position(glm::vec3 _local_position) override;
	void set_rotation(quat _rotation) override;
	void set_scale(glm::vec3 _scale) override;

	glm::mat4 camera_to_clip();
	glm::mat4 world_to_clip();

	glm::vec3 forward();
	glm::vec3 up();
	glm::vec3 right();

	glm::vec4 ZBufferParams();

private:
	int prev_mouse_x;
	int prev_mouse_y;
	bool locked;
};
