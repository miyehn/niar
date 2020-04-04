#pragma once
#include "lib.h"

struct Ray;
struct Triangle;

struct Light {
	virtual ~Light() {}
	virtual float ray_to_light(Ray& ray, const vec3& origin) = 0;
	virtual vec3 get_emission() = 0;
};

struct AreaLight : public Light {
	AreaLight(const Triangle* _triangle) : triangle(_triangle) {}
	virtual ~AreaLight() {}
	const Triangle* triangle;
	vec3 get_emission();
	float ray_to_light(Ray& ray, const vec3& origin);
};
