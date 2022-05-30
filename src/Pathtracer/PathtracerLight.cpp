#include "PathtracerLight.hpp"
#include "Primitive.hpp"
#include "BSDF.hpp"

using namespace glm;

PathtracerMeshLight::PathtracerMeshLight(Triangle* _triangle)
	: triangle(_triangle)
{
	type = PathtracerLight::Mesh;
}

vec3 PathtracerMeshLight::get_emission() {
	return triangle->bsdf->get_emission();
}

float PathtracerMeshLight::ray_to_light_pdf(Ray& ray, const vec3 &origin) {
	vec3 light_p = triangle->sample_point();

	ray.o = origin;
	ray.d = normalize(light_p - origin);
	double t; vec3 n;
	triangle->intersect(ray, t, n, true);

	double d2 = t * t;

	float costheta_l = std::max(0.0f, dot(-ray.d, n)); // non-negative

	double eps_adjusted = EPSILON / costheta_l;
	ray.tmin = eps_adjusted;
	ray.tmax -= eps_adjusted;

	// could be 0 or infinite
	return d2 / (triangle->area * costheta_l);
}

PathtracerPointLight::PathtracerPointLight(const glm::vec3& in_position, const glm::vec3& in_emission)
	: position(in_position), emission(in_emission)
{
	type = PathtracerLight::Point;
}

glm::vec3 PathtracerPointLight::get_emission() {
	return emission;
}

float PathtracerPointLight::ray_to_light_pdf(Ray &ray, const vec3 &origin) {
	ray.o = origin;
	vec3 path = position - origin;
	double path_len = length(path);
	ray.d = normalize(path);

	ray.tmin = EPSILON;
	ray.tmax = path_len - 2 * EPSILON;

	return (float)(path_len * path_len * 4 * PI);
}
