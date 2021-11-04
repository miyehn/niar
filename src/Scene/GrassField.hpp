#pragma once

#include "Engine/SceneObject.hpp"

struct Blade{
	Blade(glm::vec3 root);
	glm::vec4 root_w; // v0, width
	glm::vec4 above_h; // v1, height
	glm::vec4 ctrl_s; // v2, stiffness
	glm::vec4 up_o; // a unit vector up, orientation
};
static_assert(sizeof(Blade) == 16 * sizeof(float), "Blade should be packed");

struct GrassField : public SceneObject {
	
	explicit GrassField(
			uint32_t num_blades,
			SceneObject* _parent = nullptr,
			std::string _name = "grass");

	// inherited
	void update(float elapsed) override;
	bool handle_event(SDL_Event event) override;
	void draw() override;

	void set_local_position(glm::vec3 _local_position) override;
	void set_rotation(glm::quat _rotation) override;
	void set_scale(glm::vec3 _scale) override;

	// properties and methods
	std::vector<Blade> blades;
};

