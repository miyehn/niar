#include "BVH.hpp"

inline float BVH::surface_area() {
    if (primitives.size() == 0) return 0.0f;
    return (max.x - min.x) * (max.y - min.y) * (max.z - min.z);
}

void BVH::add_primitive(Primitive* prim) {
    primitives.push_back(prim);
}

void BVH::build_bvh() {
    TRACE("built BVH.");
}

Primitive* BVH::intersect_primitives(Ray& ray, double& t, vec3& n) {
    Primitive* primitive = nullptr;
	for (size_t i = 0; i < primitives.size(); i++) {
		Primitive* prim_tmp = primitives[i]->intersect(ray, t, n, true);
		if (prim_tmp) {
			primitive = prim_tmp;
		}
	}
    return primitive;
}