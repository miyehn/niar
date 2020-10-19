#include "Drawable.hpp"
#include "Materials.hpp"
#include "Camera.hpp"

MatGeneric* Material::mat_depth_value = nullptr;
MatGeneric* Material::mat_depth() {
	if (mat_depth_value) return mat_depth_value;
	mat_depth_value = new MatGeneric("depth");
	return mat_depth_value;
}

void Material::cleanup() {
	if (mat_depth_value) delete mat_depth_value;
}

//------------------------------------------------

void Material::use(const Drawable* obj) {
	glUseProgram(shader->id);
	set_parameters();
}

#define IMPLEMENT_MATERIAL(NAME) \
	void NAME::use(const Drawable* obj) { \
		Material::use(obj);

#define END_IMPLEMENT_MATERIAL }

IMPLEMENT_MATERIAL(MatGeneric)
	mat4 o2w = obj->object_to_world();
	mat3 o2wr = obj->object_to_world_rotation();

	mat4 w2c = Camera::Active->world_to_camera();
	mat3 w2cr = Camera::Active->world_to_camera_rotation();
	mat4 w2cl = Camera::Active->world_to_clip();

	// set transformation matrices
	shader->set_mat4("OBJECT_TO_WORLD", o2w, true);
	shader->set_mat3("OBJECT_TO_WORLD_ROT", o2wr, true);
	shader->set_mat4("OBJECT_TO_CAM", w2c * o2w, true);
	shader->set_mat3("OBJECT_TO_CAM_ROT", w2cr * o2wr, true);
	shader->set_mat4("OBJECT_TO_CLIP", w2cl * o2w, true);
	shader->set_mat4("WORLD_TO_CLIP", w2cl, true);
END_IMPLEMENT_MATERIAL

IMPLEMENT_MATERIAL(MatBasic)
	// transformation 
	mat4 o2w = obj->object_to_world();
	mat3 o2wr = obj->object_to_world_rotation();

	mat3 w2cr = Camera::Active->world_to_camera_rotation();
	mat4 w2cl = Camera::Active->world_to_clip();

	shader->set_mat3("OBJECT_TO_CAM_ROT", w2cr * o2wr);
	shader->set_mat4("OBJECT_TO_CLIP", w2cl * o2w);

	// others
	shader->set_tex2D("BaseColor", 0, base_color->id());
	shader->set_vec3("Color", tint);
END_IMPLEMENT_MATERIAL

IMPLEMENT_MATERIAL(MatDeferredGeometryBasic)
	// transformation 
	mat4 o2w = obj->object_to_world();
	mat3 o2wr = obj->object_to_world_rotation();

	mat4 w2cl = Camera::Active->world_to_clip();

	shader->set_mat3("OBJECT_TO_WORLD_ROT", o2wr);
	shader->set_mat4("OBJECT_TO_WORLD", o2w);
	shader->set_mat4("OBJECT_TO_CLIP", w2cl * o2w);

	// others
	shader->set_tex2D("BaseColor", 0, base_color->id());
	shader->set_vec3("Tint", tint);
END_IMPLEMENT_MATERIAL

IMPLEMENT_MATERIAL(MatDeferredGeometry)
	// transformation 
	mat4 o2w = obj->object_to_world();
	mat3 o2wr = obj->object_to_world_rotation();

	mat4 w2cl = Camera::Active->world_to_clip();

	shader->set_mat3("OBJECT_TO_WORLD_ROT", o2wr);
	shader->set_mat4("OBJECT_TO_WORLD", o2w);
	shader->set_mat4("OBJECT_TO_CLIP", w2cl * o2w);

	// others
	shader->set_tex2D("BaseColor", 0, base_color->id());
	shader->set_tex2D("NormalMap", 1, normal_map->id());
	shader->set_vec3("Tint", tint);
END_IMPLEMENT_MATERIAL

IMPLEMENT_MATERIAL(MatGrass)
	mat4 o2w = obj->object_to_world();
	mat4 w2cl = Camera::Active->world_to_clip();

	shader->set_mat4("transformation", w2cl * o2w);
END_IMPLEMENT_MATERIAL
