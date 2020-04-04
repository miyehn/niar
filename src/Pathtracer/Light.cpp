#include "Light.hpp"
#include "Triangle.hpp"

AreaLight::AreaLight(const Triangle* _triangle)
	: triangle(_triangle) {
}

vec3 AreaLight::get_emission() {
	return triangle->bsdf->Le;
}

float AreaLight::ray_to_light_pdf(Ray& ray, const vec3 &origin) {
	vec3 light_p = triangle->sample_point();

	ray.o = origin;
	ray.d = normalize(light_p - origin);
	float t; vec3 n;
	triangle->intersect(ray, t, n);

	float distance = ray.tmax;

	// but move its end back a bit so it doesn't really hit the light (just almost)
	ray.tmax = ray.tmax - EPSILON;

	// TODO: should area lights be two-sided like this?
	float costheta_l = abs(dot(ray.d, n));

	return distance * distance / (triangle->area * costheta_l);
}
