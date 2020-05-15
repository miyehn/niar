#pragma once
#include "Drawable.hpp"

struct Ray;
struct Triangle;
struct Camera;

struct Light : public Drawable {
	enum Type { Area, Point, Directional };
	Type type;
	virtual ~Light() {}
	virtual vec3 get_emission() = 0;
	virtual void render_shadow_map() = 0;
};

// for pathtracer only
struct AreaLight : public Light {

	AreaLight(Triangle* _triangle);
	virtual ~AreaLight() {}

	Triangle* triangle;

	vec3 get_emission();

	// returns pdf for sampling this particular ray among A' (area projected onto hemisphere)
	float ray_to_light_pdf(Ray& ray, const vec3& origin);

	virtual void render_shadow_map() { ERR("Area lights shouldn't need shadow maps"); }
};

struct DirectionalLight : public Light {

	DirectionalLight(
			vec3 _color = vec3(1), 
			float _intensity = 1.0f, 
			vec3 dir = vec3(0, 0, -1));

	void set_direction(vec3 dir) { rotation = quat_from_dir(normalize(dir)); }

	virtual vec3 get_emission() { return color * intensity; }

	vec3 get_direction() { return object_to_world_rotation() * vec3(0, 0, 1); }

	float effective_radius = 24.0f;

	virtual void render_shadow_map();

private:
	vec3 color;
	float intensity;

	const uint shadow_map_dim = 1024;
	Camera* shadow_map_cam = nullptr;
	Blit* shadow_map_blit = nullptr;
};

struct PointLight: public Light {

	PointLight(
			vec3 _color = vec3(1), 
			float _intensity = 1.0f, 
			vec3 _local_pos = vec3(0)) : 
		color(_color), intensity(_intensity) {
		local_position = _local_pos; 
	} // TODO: why

	virtual vec3 get_emission() { return color * intensity; }

	virtual void render_shadow_map();

private:
	vec3 color;
	float intensity;
};
