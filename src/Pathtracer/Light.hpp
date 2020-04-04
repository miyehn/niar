#pragma once
#include "lib.h"

struct Ray;
struct Triangle;

struct Light {
	virtual ~Light() {}
	virtual Ray ray_to_light(const vec3& origin) = 0;
	virtual vec3 get_emission() = 0;
};

struct AreaLight : public Light {
	AreaLight(const Triangle* _triangle) : triangle(_triangle) {}
	virtual ~AreaLight() {}
	const Triangle* triangle;
	vec3 get_emission();
	Ray ray_to_light(const vec3& origin);
};
