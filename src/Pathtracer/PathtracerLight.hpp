#pragma once
#include <glm/glm.hpp>

struct Ray;
struct Triangle;

struct PathtracerLight {

	enum Type {
		Mesh,
		Point,
		Directional
	};

	Type type;

	virtual ~PathtracerLight() = default;

	virtual glm::vec3 get_emission() = 0;

	virtual float ray_to_light_pdf(Ray& ray, const glm::vec3& origin) = 0;
};

struct PathtracerMeshLight : public PathtracerLight {

	explicit PathtracerMeshLight(Triangle* _triangle);
	~PathtracerMeshLight() override = default;

	Triangle* triangle;

	glm::vec3 get_emission() override;

	// returns pdf for sampling this particular ray among A' (area projected onto hemisphere)
	float ray_to_light_pdf(Ray& ray, const glm::vec3& origin) override;
};

struct PathtracerPointLight : public PathtracerLight {

	explicit PathtracerPointLight(const glm::vec3& position, const glm::vec3& emission);
	~PathtracerPointLight() override = default;

	glm::vec3 position;
	glm::vec3 emission;

	glm::vec3 get_emission() override { return emission; }

	// returns pdf for sampling this particular ray among A' (area projected onto hemisphere)
	float ray_to_light_pdf(Ray& ray, const glm::vec3& origin) override;
};

struct PathtracerDirectionalLight : public PathtracerLight {

	explicit PathtracerDirectionalLight(const glm::vec3& direction, const glm::vec3& emission);
	~PathtracerDirectionalLight() override = default;

	glm::vec3 direction;
	glm::vec3 emission;

	glm::vec3 get_emission() override { return emission; }

	// returns pdf for sampling this particular ray among A' (area projected onto hemisphere)
	float ray_to_light_pdf(Ray& ray, const glm::vec3& origin) override;
};