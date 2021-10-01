#pragma once
#include "Engine/Drawable.hpp"

struct Scene;
struct Camera;
struct Mesh;
struct aiLight;
struct aiNode;
struct MatGeneric;

struct Light : public Drawable {

	enum Type { Point, Directional };
	Type type;
	virtual ~Light();
	vec3 get_emission() { return color * intensity; }
	virtual void render_shadow_map() = 0;

	virtual void set_cast_shadow(bool cast) = 0;
	bool get_cast_shadow() { return cast_shadow; }

	uint get_shadow_map(){ return shadow_map_tex; }
	uint get_shadow_mask(){ return shadow_mask_tex; }

	virtual void set_local_position(vec3 _local_position) { local_position_value = _local_position; }
	virtual void set_rotation(quat _rotation) { rotation_value = _rotation; }
	virtual void set_scale(vec3 _scale) { scale_value = _scale; }

protected:
	vec3 color;
	float intensity;

	bool cast_shadow = false;
	bool shadow_map_initialized = false;
	uint shadow_map_dim;

	//-------- opengl stuff --------
	uint shadow_map_tex = 0; // GL_TEXTURE_2D || GL_TEXTURE_CUBE_MAP
	Camera* shadow_map_cam = nullptr;
	uint shadow_mask_tex = 0;

};

struct DirectionalLight : public Light {

	DirectionalLight(
			vec3 _color = vec3(1), 
			float _intensity = 1.0f, 
			vec3 dir = vec3(0, 0, -1));
	DirectionalLight(aiLight* light, aiNode* mRootNode);

	virtual ~DirectionalLight();

	virtual void set_cast_shadow(bool cast);

	void set_direction(vec3 dir) { set_rotation(quat_from_dir(normalize(dir))); }

	vec3 get_direction() { return object_to_world_rotation() * vec3(0, 0, -1); }

	virtual void render_shadow_map();

	mat4 world_to_light_clip();

private:

	void init(vec3 _color, float _intensity, vec3 dir);

	uint shadow_map_fbo = 0;

};

struct PointLight: public Light {

	PointLight(
			vec3 _color = vec3(1), 
			float _intensity = 1.0f, 
			vec3 _local_pos = vec3(0));

	PointLight(aiLight* light, aiNode* mRootNode);

	virtual ~PointLight();

	virtual void render_shadow_map();

	virtual void set_cast_shadow(bool cast);

private:

	void init(vec3 _color, float _intensity, vec3 _local_pos);

	uint shadow_map_fbos[6];
	vec3 shadow_map_normals[6] = {
		vec3(1, 0, 0), vec3(-1, 0, 0),
		vec3(0, 1, 0), vec3(0, -1, 0),
		vec3(0, 0, 1), vec3(0, 0, -1)
	};
	uint shadow_map_depth_rbo = 0;

	MatGeneric* distance_to_light_mat = nullptr;

};
