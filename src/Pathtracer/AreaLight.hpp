#pragma once
#include "Light.hpp"

struct Ray;
struct Triangle;

struct AreaLight : public Light {

	AreaLight(Triangle* _triangle);
	virtual ~AreaLight() {}

	Triangle* triangle;

	vec3 get_emission();

	// returns pdf for sampling this particular ray among A' (area projected onto hemisphere)
	float ray_to_light_pdf(Ray& ray, const vec3& origin);

	virtual void render_shadow_map() {}

	virtual mat4 world_to_light_clip() { return mat4(1); }
};

