#include "Light.hpp"
#include "Triangle.hpp"

vec3 AreaLight::get_emission() {
	return triangle->bsdf->Le;
}

Ray AreaLight::ray_to_light(const vec3 &origin) {
	vec3 light_p = triangle->sample_point();

	Ray ray(origin, normalize(light_p - origin));
	float t; vec3 n;
	triangle->intersect(ray, t, n);
	// but move its end back a bit so it doens't really hit the light (just almost)
	ray.tmax = ray.tmax - 0.1f;

	return ray;
}
