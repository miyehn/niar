#include "BVH.hpp"

#define BVH_THRESHOLD 8
#define PER_AXIS_GRANULARITY 8

inline float BVH::surface_area() {
	if (primitives.size() == 0) return 0.0f;
	return (max.x - min.x) * (max.y - min.y) * (max.z - min.z);
}

void BVH::add_primitive(Primitive* prim)
{
	Triangle* T = dynamic_cast<Triangle*>(prim);
	if (!T) return; // currently bvh only supports triangles
	for (int i=0; i<3; i++) {
		min = glm::min(min, T->vertices[i]);
		max = glm::max(max, T->vertices[i]);
	}
	primitives.push_back(prim);
}

void BVH::expand_bvh()
{
	if (expanded) return;
	expanded = true;

	if (primitives.size() <= BVH_THRESHOLD) return;

	vec3 step = (max - min) * (1.0f / PER_AXIS_GRANULARITY);

	float min_SA = INF;

	// X axis: try the K ways to divide it
	for (int i=1; i<PER_AXIS_GRANULARITY; i++)
	{
		float x_divide = min.x + i * step.x;
		BVH* left_tmp = new BVH();
		BVH* right_tmp = new BVH();

		// tentatively make two subtrees
		for (Primitive* P : primitives)
		{
			Triangle* T = dynamic_cast<Triangle*>(P);
			float x_center = (T->vertices[0].x + T->vertices[1].x + T->vertices[2].x) / 3.0f;
			if (x_center < x_divide) left_tmp->add_primitive(P);
			else right_tmp->add_primitive(P);
		}

		// check if it's better than the optimal division found so far
		bool accepted = false;
		if (left_tmp->primitives.size()>0 && right_tmp->primitives.size()>0)
		{
			float SA = left_tmp->surface_area() + right_tmp->surface_area();
			if (SA < min_SA) {
				left = left_tmp;
				right = right_tmp;
				accepted = true;
				min_SA = SA;
			}
		}
		if (!accepted) {
			delete left_tmp;
			delete right_tmp;
		}
	}

	// Y axis
	for (int i=1; i<PER_AXIS_GRANULARITY; i++)
	{
		float y_divide = min.y + i * step.y;
		BVH* left_tmp = new BVH();
		BVH* right_tmp = new BVH();

		// tentatively make two subtrees
		for (Primitive* P : primitives)
		{
			Triangle* T = dynamic_cast<Triangle*>(P);
			float y_center = (T->vertices[0].y + T->vertices[1].y + T->vertices[2].y) / 3.0f;
			if (y_center < y_divide) left_tmp->add_primitive(P);
			else right_tmp->add_primitive(P);
		}

		// check if it's better than the optimal division found so far
		bool accepted = false;
		if (left_tmp->primitives.size()>0 && right_tmp->primitives.size()>0)
		{
			float SA = left_tmp->surface_area() + right_tmp->surface_area();
			if (SA < min_SA) {
				left = left_tmp;
				right = right_tmp;
				accepted = true;
				min_SA = SA;
			}
		}
		if (!accepted) {
			delete left_tmp;
			delete right_tmp;
		}
	}
	// Z axis
	for (int i=1; i<PER_AXIS_GRANULARITY; i++)
	{
		float z_divide = min.z + i * step.z;
		BVH* left_tmp = new BVH();
		BVH* right_tmp = new BVH();

		// tentatively make two subtrees
		for (Primitive* P : primitives)
		{
			Triangle* T = dynamic_cast<Triangle*>(P);
			float z_center = (T->vertices[0].z + T->vertices[1].z + T->vertices[2].z) / 3.0f;
			if (z_center < z_divide) left_tmp->add_primitive(P);
			else right_tmp->add_primitive(P);
		}

		// check if it's better than the optimal division found so far
		bool accepted = false;
		if (left_tmp->primitives.size()>0 && right_tmp->primitives.size()>0)
		{
			float SA = left_tmp->surface_area() + right_tmp->surface_area();
			if (SA < min_SA) {
				left = left_tmp;
				right = right_tmp;
				accepted = true;
				min_SA = SA;
			}
		}
		if (!accepted) {
			delete left_tmp;
			delete right_tmp;
		}
	}

	// need this check because there could be cases when all divisions result in one child being empty.
	// In that case make this node a leaf.
	if (left && right) {
		left->expand_bvh();
		right->expand_bvh();
	} else {
		WARN("!!!");
	}
}
   
 
bool BVH::intersect_aabb(Ray& ray, float& distance)
{

}

Primitive* BVH::intersect_primitives(Ray& ray, double& t, vec3& n) 
{
	if (!expanded) {
		Primitive* primitive = nullptr;
		for (size_t i = 0; i < primitives.size(); i++) {
			Primitive* prim_tmp = primitives[i]->intersect(ray, t, n, true);
			if (prim_tmp) {
				primitive = prim_tmp;
			}
		}
		return primitive;
	}

	Primitive* primitive = nullptr;
#if 0
	for (size_t i = 0; i < primitives.size(); i++) {
		Primitive* prim_tmp = primitives[i]->intersect(ray, t, n, true);
		if (prim_tmp) {
			primitive = prim_tmp;
		}
	}
#else
	BVH* tmp = right;
	for (int i = 0; i < tmp->primitives.size(); i++) {
		Primitive* prim_tmp = tmp->primitives[i]->intersect(ray, t, n, true);
		if (prim_tmp) {
			primitive = prim_tmp;
		}
	}
#endif
	return primitive;
}