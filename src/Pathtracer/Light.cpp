#include "Light.hpp"
#include "Triangle.hpp"

vec3 AreaLight::get_emission() {
	return triangle->bsdf->Le;
}

float AreaLight::ray_to_light(Ray& ray, const vec3 &origin) {
	vec3 light_p = triangle->sample_point();

	ray.o = origin;
	ray.d = normalize(light_p - origin);
	float t; vec3 n;
	triangle->intersect(ray, t, n);

	// but move its end back a bit so it doens't really hit the light (just almost)
	ray.tmax = ray.tmax - 0.0001f;

	return 1.0f;
}
