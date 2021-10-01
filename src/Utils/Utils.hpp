#pragma once
#include "lib.h"

struct Scene;

//---------------------------------------------------------

struct AABB {
	AABB() {
		min = vec3(INF);
		max = vec3(-INF);
	}
	void add_point(vec3 p);
	void merge(const AABB& other);
	static AABB merge(const AABB& A, const AABB& B);
	std::vector<vec3> corners();
	std::string str() { return "min: " + s3(min) + ", max: " + s3(max); }

	vec3 min, max;
};

//---------------------------------------------------------

namespace f {

bool import_scene(Scene* scene, const char* path);

}
