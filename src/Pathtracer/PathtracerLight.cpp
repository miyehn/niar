#include "PathtracerLight.hpp"
#include "Primitive.hpp"
#include "BSDF.hpp"

using namespace glm;

namespace {
float luminance(const vec3& col) {
	return 0.2126f * col.r + 0.7152f * col.g + 0.0722 * col.b;
}
};

PathtracerMeshLight::PathtracerMeshLight(Triangle* _triangle)
	: triangle(_triangle)
{
	type = PathtracerLight::Mesh;
}

vec3 PathtracerMeshLight::get_emission() {
	return triangle->bsdf->get_emission();
}

void PathtracerMeshLight::ray_to_light_and_attenuation(Ray &ray, float &attenuation) {
	vec3 light_p = triangle->sample_point();

	ray.d = normalize(light_p - ray.o);
	double t; vec3 n;
	triangle->intersect(ray, t, n, true);

	double d2 = t * t;

	float costheta_l = std::max(0.0f, dot(-ray.d, n)); // non-negative

	double eps_adjusted = EPSILON / costheta_l;
	ray.tmin = eps_adjusted;
	ray.tmax -= eps_adjusted;

	// could be 0 or infinite
	attenuation = (triangle->area * costheta_l) / d2;
}

float PathtracerMeshLight::get_weight() {
	return luminance(get_emission());
}

PathtracerPointLight::PathtracerPointLight(const glm::vec3& in_position, const glm::vec3& in_emission)
	: position(in_position), emission(in_emission)
{
	type = PathtracerLight::Point;
}

void PathtracerPointLight::ray_to_light_and_attenuation(Ray &ray, float &attenuation) {
	vec3 path = position - ray.o;
	double path_len = length(path);
	ray.d = normalize(path);

	ray.tmin = EPSILON;
	ray.tmax = path_len - 2 * EPSILON;

	attenuation = 1.0f / (float)(path_len * path_len * 4 * PI);
}

float PathtracerPointLight::get_weight() {
	return luminance(get_emission() / (4 * PI));
}

PathtracerDirectionalLight::PathtracerDirectionalLight(const vec3 &in_direction, const vec3 &in_emission)
	: direction(in_direction), emission(in_emission)
{
	type = PathtracerLight::Directional;
}

void PathtracerDirectionalLight::ray_to_light_and_attenuation(Ray &ray, float &attenuation) {
	ray.d = -direction;
	ray.tmin = EPSILON;
	ray.tmax = INF;
	attenuation = 1;
}

float PathtracerDirectionalLight::get_weight() {
	return luminance(get_emission());
}
