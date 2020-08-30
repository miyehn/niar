#include "Drawable.hpp"
#include "Materials.hpp"
#include "Camera.hpp"

Material* Material::mat_depth_value = nullptr;
Material* Material::mat_depth() {
	if (mat_depth_value) return mat_depth_value;
	mat_depth_value = new MatGeneric("depth");
	mat_depth_value->shader->set_static_parameters = []() {};
	return mat_depth_value;
}

void Material::use(const Drawable* obj) {
	glUseProgram(shader->id);
	shader->set_static_parameters();
}

void MatGeneric::use(const Drawable* obj) {
	Material::use(obj);

	mat4 o2w = obj->object_to_world();
	mat3 o2wr = obj->object_to_world_rotation();

	mat4 w2c = Camera::Active->world_to_camera();
	mat3 w2cr = Camera::Active->world_to_camera_rotation();
	mat4 w2cl = Camera::Active->world_to_clip();

	// set those
	shader->set_mat4("OBJECT_TO_WORLD", o2w, true);
	shader->set_mat3("OBJECT_TO_WORLD_ROT", o2wr, true);
	shader->set_mat4("OBJECT_TO_CAM", w2c * o2w, true);
	shader->set_mat3("OBJECT_TO_CAM_ROT", w2cr * o2wr, true);
	shader->set_mat4("OBJECT_TO_CLIP", w2cl * o2w, true);
	
}
