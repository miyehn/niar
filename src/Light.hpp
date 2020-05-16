#pragma once
#include "Drawable.hpp"

struct Camera;

struct Light : public Drawable {
	enum Type { Area, Point, Directional };
	Type type;
	virtual ~Light() {}
	virtual vec3 get_emission() = 0;
	virtual void render_shadow_map() = 0;
	virtual mat4 world_to_light_clip() = 0;
	bool cast_shadow = false;
	float effective_radius = 10.0f;

	uint get_shadow_map(){ return shadow_map_tex; }
	uint get_shadow_mask(){ return shadow_mask_tex; }

protected:
	//-------- opengl stuff --------
	uint shadow_map_fbo = 0;
	uint shadow_map_tex = 0;
	Camera* shadow_map_cam = nullptr;
	uint shadow_mask_tex = 0;

};

struct DirectionalLight : public Light {

	DirectionalLight(
			vec3 _color = vec3(1), 
			float _intensity = 1.0f, 
			vec3 dir = vec3(0, 0, -1));

	virtual ~DirectionalLight();

	void set_direction(vec3 dir) { rotation = quat_from_dir(normalize(dir)); }

	virtual vec3 get_emission() { return color * intensity; }

	vec3 get_direction() { return object_to_world_rotation() * vec3(0, 0, -1); }

	virtual void render_shadow_map();

	virtual mat4 world_to_light_clip();

private:
	vec3 color;
	float intensity;

	const uint shadow_map_dim = 1024;
	Blit* shadow_map_blit = nullptr;

};

struct PointLight: public Light {

	PointLight(
			vec3 _color = vec3(1), 
			float _intensity = 1.0f, 
			vec3 _local_pos = vec3(0)) : 
		color(_color), intensity(_intensity) {
		local_position = _local_pos; 
	}

	virtual vec3 get_emission() { return color * intensity; }

	virtual void render_shadow_map();

	virtual mat4 world_to_light_clip() { return mat4(1); }

private:
	vec3 color;
	float intensity;
};
