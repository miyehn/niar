#include "AABB.hpp"
#include "Scene/Scene.hpp"
#include "Render/Mesh.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic pop

using namespace glm;

//------------------ AABB -------------------------

void AABB::add_point(vec3 p) {
	min.x = glm::min(min.x, p.x);
	min.y = glm::min(min.y, p.y);
	min.z = glm::min(min.z, p.z);
	max.x = glm::max(max.x, p.x);
	max.y = glm::max(max.y, p.y);
	max.z = glm::max(max.z, p.z);
}

void AABB::merge(const AABB& other) {
	min.x = glm::min(min.x, other.min.x);
	min.y = glm::min(min.y, other.min.y);
	min.z = glm::min(min.z, other.min.z);
	max.x = glm::max(max.x, other.max.x);
	max.y = glm::max(max.y, other.max.y);
	max.z = glm::max(max.z, other.max.z);
}

AABB AABB::merge(const AABB& A, const AABB& B) {
	AABB res;
	res.merge(A);
	res.merge(B);
	return res;
}

std::vector<vec3> AABB::corners() {
	std::vector<vec3> res(8);
	res[0] = min;
	res[1] = vec3(max.x, min.y, min.z);
	res[2] = vec3(min.x, max.y, min.z);
	res[3] = vec3(max.x, max.y, min.z);
	res[4] = vec3(min.x, min.y, max.z);
	res[5] = vec3(max.x, min.y, max.z);
	res[6] = vec3(min.x, max.y, max.z);
	res[7] = max;
	return res;
}

//-------------------------------------------------
