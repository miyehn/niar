#include "PathtracerLight.hpp"
#include "Primitive.hpp"
#include "BSDF.hpp"

AreaLight::AreaLight(Triangle* _triangle)
	: triangle(_triangle) {
}

vec3 AreaLight::get_emission() {
	return triangle->bsdf->get_emission();
}

float AreaLight::ray_to_light_pdf(Ray& ray, const vec3 &origin) {
	vec3 light_p = triangle->sample_point();

	ray.o = origin;
	ray.d = normalize(light_p - origin);
	double t; vec3 n;
	triangle->intersect(ray, t, n, true);

	double d2 = t * t;

	float costheta_l = std::max(0.0f, dot(-ray.d, n)); // non-negative

	// could be 0 or infinite
	return d2 / (triangle->area * costheta_l);
}

