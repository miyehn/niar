#pragma once
#include "Engine/SceneObject.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

typedef std::vector<glm::vec3> Frustum;

namespace tinygltf { struct Camera; }

struct Camera : SceneObject {

	static Camera* Active;

	explicit Camera(uint32_t w = 0, uint32_t h = 0, bool _ortho = false);
	explicit Camera(const std::string& node_name, const tinygltf::Camera*);

#if GRAPHICS_DISPLAY
	void update_control(float time_elapsed);
#endif

	// properties, can be set by the program

#if GRAPHICS_DISPLAY
	float move_speed;
	float rotate_speed;
#endif

	float fov;
	float cutoffNear;
	float cutoffFar;

	float width, height;
	float aspect_ratio;

	bool orthographic;

	Frustum frustum();

#if GRAPHICS_DISPLAY
	// can move & rotate camera?
	void lock();
	void unlock();

	// functions

	void draw_config_ui() override;
#endif

	void set_local_position(glm::vec3 _local_position) override;
	void set_rotation(glm::quat _rotation) override;
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
