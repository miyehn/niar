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
   
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection 
// TODO: write a more readable version of this
bool BVH::intersect_aabb(const Ray& ray, float& distance)
{
	float tmin = (min.x - ray.o.x) / ray.d.x; 
	float tmax = (max.x - ray.o.x) / ray.d.x; 
 
	if (tmin > tmax) std::swap(tmin, tmax); 
 
	float tymin = (min.y - ray.o.y) / ray.d.y; 
	float tymax = (max.y - ray.o.y) / ray.d.y; 
 
	if (tymin > tymax) std::swap(tymin, tymax); 
 
	if ((tmin > tymax) || (tymin > tmax)) 
		return false; 
 
	if (tymin > tmin) 
		tmin = tymin; 
 
	if (tymax < tmax) 
		tmax = tymax; 
 
	float tzmin = (min.z - ray.o.z) / ray.d.z; 
	float tzmax = (max.z - ray.o.z) / ray.d.z; 
 
	if (tzmin > tzmax) std::swap(tzmin, tzmax); 
 
	if ((tmin > tzmax) || (tzmin > tmax)) 
		return false; 
 
	if (tzmin > tmin) 
		tmin = tzmin; 
 
	if (tzmax < tmax) 
		tmax = tzmax; 

	if (tmin >= 0) distance = tmin;
	else distance = tmax;
 
	return true; 
	
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
	float d_box;
	if (intersect_aabb(ray, d_box))
	{
		if (left || right)
		{

#if 0
			float d_left, d_right;
			if (left->intersect_aabb(ray, d_left))
			{
				if (right->intersect_aabb(ray, d_right)) // intersected both bboxes, go with the closer one
				{
					BVH* first = nullptr;
					BVH* second = nullptr;
					float d_near, d_far;
					if (d_left < d_right) {
						first = left; second = right;
						d_near = d_left; d_far = d_right;
					} else {
						first = right; second = left;
						d_near = d_right; d_far = d_left;
					}

					primitive = first->intersect_primitives(ray, t, n);
					if (!primitive || t > d_far) {
						Primitive* prim_tmp = second->intersect_primitives(ray, t, n);
						if (prim_tmp) {
							primitive = prim_tmp;
						}
					}
				}
				else // only intersected with left
				{
					primitive = left->intersect_primitives(ray, t, n);
				}
			}
			else // only intersected with right
			{
				primitive = right->intersect_primitives(ray, t, n);
			}
#else
			primitive = left->intersect_primitives(ray, t, n);
			Primitive* prim_tmp = right->intersect_primitives(ray, t, n);
			if (prim_tmp) primitive = prim_tmp;
#endif
		}
		else
		{
			for (size_t i = 0; i < primitives.size(); i++) {
				Primitive* prim_tmp = primitives[i]->intersect(ray, t, n, true);
				if (prim_tmp) {
					primitive = prim_tmp;
				}
			}
		}
	}
#endif
	return primitive;
}