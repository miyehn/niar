#include "GrassField.hpp"

using namespace glm;

GrassField::GrassField(uint32_t num_blades, SceneObject* _parent, std::string _name): SceneObject(_parent, _name) {

	// create blades
	blades = std::vector<Blade>();
	for (uint32_t i=0; i<num_blades; i++) {
		Blade b = Blade(vec3(-10.0f + i*3.0f, 0.0f, 0.0f));
		blades.push_back(b);
	}
}

void GrassField::update(float elapsed) {
	SceneObject::update(elapsed);
}

bool GrassField::handle_event(SDL_Event event) {
	return SceneObject::handle_event(event);
}

void GrassField::draw() {

}

void GrassField::set_local_position(vec3 _local_position) {
	local_position_value = _local_position;
	/*
	generate_aabb();
	get_scene()->generate_aabb();
	*/
}

void GrassField::set_rotation(quat _rotation) {
	rotation_value = _rotation;
	/*
	generate_aabb();
	get_scene()->generate_aabb();
	*/
}

void GrassField::set_scale(vec3 _scale) {
	scale_value = _scale;
	/*
	generate_aabb();
	get_scene()->generate_aabb();
	*/
}


Blade::Blade(vec3 root) {
	this->up_o = vec4(0.0f, 0.0f, 1.0f, radians(65.0f));

	this->root_w = vec4(root.x, root.y, root.z, 0.65f);

	this->above_h = this->up_o * 4.0f;
	this->above_h.w = 1.0f;

	this->ctrl_s = above_h + vec4(-0.8f, 0.0f, 0.0f, 0.0f);
	this->ctrl_s.w = 1.0f;
}
