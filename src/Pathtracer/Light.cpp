#include "Light.hpp"
#include "Primitive.hpp"
#include "BSDF.hpp"

AreaLight::AreaLight(Triangle* _triangle)
	: triangle(_triangle) {
	type = Area;
}

vec3 AreaLight::get_emission() {
	return triangle->bsdf->Le;
}

float AreaLight::ray_to_light_pdf(Ray& ray, const vec3 &origin) {
	vec3 light_p = triangle->sample_point();

	ray.o = origin;
	ray.d = normalize(light_p - origin);
	double t; vec3 n;
	triangle->intersect(ray, t, n, true);

	double d2 = t * t;

	// TODO: make more robust
	float costheta_l = dot(-ray.d, n);

	return d2 / (triangle->area * costheta_l);
}

