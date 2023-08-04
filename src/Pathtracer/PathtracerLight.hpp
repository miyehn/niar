#pragma once
#include <glm/glm.hpp>

struct Ray;
struct Triangle;

namespace myn::sky{ class CpuSkyAtmosphere; }

class PathtracerLight {
public:

	virtual ~PathtracerLight() = default;
	bool is_delta() { return _is_delta; }

	virtual float get_weight() = 0;
	virtual glm::vec3 get_emission() = 0;
	virtual void ray_to_light_and_attenuation(Ray& ray, float& attenuation) = 0;

protected:
	bool _is_delta;
};

class PathtracerMeshLight : public PathtracerLight {
public:
	explicit PathtracerMeshLight(Triangle* _triangle);
	~PathtracerMeshLight() override = default;

	float get_weight() override;
	glm::vec3 get_emission() override;

	// atten considers pdf for sampling this particular ray among A' (area projected onto hemisphere)
	void ray_to_light_and_attenuation(Ray& ray, float& attenuation) override;

	Triangle* triangle;
};

class PathtracerPointLight : public PathtracerLight {
public:
	explicit PathtracerPointLight(const glm::vec3& position, const glm::vec3& emission);
	~PathtracerPointLight() override = default;

	float get_weight() override;
	glm::vec3 get_emission() override { return emission; }

	void ray_to_light_and_attenuation(Ray& ray, float &attenuation) override;

private:
	glm::vec3 position;
	glm::vec3 emission;
};

class PathtracerDirectionalLight : public PathtracerLight {
public:
	explicit PathtracerDirectionalLight(const glm::vec3& direction, const glm::vec3& emission);
	~PathtracerDirectionalLight() override = default;

	float get_weight() override;
	glm::vec3 get_emission() override { return emission; }

	void ray_to_light_and_attenuation(Ray& ray, float &attenuation) override;

	void apply_sky(const myn::sky::CpuSkyAtmosphere* cpuSky);

private:
	glm::vec3 direction;
	glm::vec3 emission;
};