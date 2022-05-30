#pragma once
#include "Utils/myn/Misc.h"

class Scene;

struct AABB {

	AABB() {
		min = glm::vec3(INF);
		max = glm::vec3(-INF);
	}
	void add_point(glm::vec3 p);
	void merge(const AABB& other);
	static AABB merge(const AABB& A, const AABB& B);
	std::vector<glm::vec3> corners();
	std::string str() { return "min: " + myn::s3(min) + ", max: " + myn::s3(max); }

	glm::vec3 min, max;
};
