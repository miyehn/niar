#pragma once
#include "lib.h"

struct Ray;
struct Triangle;

struct Light {
	enum Type { Area, Point, Directional };
	Type type;
	virtual ~Light() {}
	virtual float ray_to_light_pdf(Ray& ray, const vec3& origin) = 0;
	virtual vec3 get_emission() = 0;
};

struct AreaLight : public Light {

	AreaLight(Triangle* _triangle);
	virtual ~AreaLight() {}

	Triangle* triangle;

	vec3 get_emission();

	// returns pdf for sampling this particular ray among A' (area projected onto hemisphere)
	float ray_to_light_pdf(Ray& ray, const vec3& origin);
};
