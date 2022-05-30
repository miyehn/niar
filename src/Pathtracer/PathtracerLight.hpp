#pragma once
#include <glm/glm.hpp>

struct Ray;
struct Triangle;

struct PathtracerLight {

	enum Type {
		AreaLight
	};

	Type type;

	virtual ~PathtracerLight() {}

	virtual glm::vec3 get_emission() = 0;

	virtual float ray_to_light_pdf(Ray& ray, const glm::vec3& origin) = 0;
};

struct AreaLight : public PathtracerLight {

	AreaLight(Triangle* _triangle);
	virtual ~AreaLight() {}

	Triangle* triangle;

	glm::vec3 get_emission() override;

	// returns pdf for sampling this particular ray among A' (area projected onto hemisphere)
	float ray_to_light_pdf(Ray& ray, const glm::vec3& origin) override;
};

