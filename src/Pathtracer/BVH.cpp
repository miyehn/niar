#include "BVH.hpp"

#define BVH_THRESHOLD 8
#define PER_AXIS_GRANULARITY 2

inline float BVH::surface_area() {
	if (primitives_count == 0) return 0.0f;
	float dx = max.x - min.x;
	float dy = max.y - min.y;
	float dz = max.z - min.z;
	return (dx * dy + dy * dz + dz * dx) * 2;
	//return (max.x - min.x) * (max.y - min.y) * (max.z - min.z);
}

void BVH::extend_primitive(Primitive* prim)
{
	Triangle* T = dynamic_cast<Triangle*>(prim);
	if (!T) return; // currently bvh only supports triangles
	for (int i=0; i<3; i++) {
		min = glm::min(min, T->vertices[i]);
		max = glm::max(max, T->vertices[i]);
	}
}

void BVH::update_extents() {
	// populate min and max:
	for (uint i=0; i<primitives_count; i++) {
		extend_primitive((*primitives_ptr)[primitives_start + i]);
	}
}

void BVH::expand_bvh()
{
	if (primitives_count <= BVH_THRESHOLD) {
		//LOGF("num prims: %u (%u -> %u)", primitives_count, primitives_start, primitives_start + primitives_count);
		return;
	}

	vec3 step = (max - min) * (1.0f / PER_AXIS_GRANULARITY);

	float min_SA = INF;
	int min_divide_axis = 0;

	auto center = [](Primitive* T_) {
		Triangle* T = dynamic_cast<Triangle*>(T_);
		return (T->vertices[0] + T->vertices[1] + T->vertices[2]) * (1.0f / 3.0f);
	};

	// x axis
	for (int i=1; i<PER_AXIS_GRANULARITY; i++)
	{
		float x_divide = min.x + i * step.x;
		BVH* left_tmp = new BVH(primitives_ptr);
		BVH* right_tmp = new BVH(primitives_ptr);
		uint left_cnt = 0, right_cnt = 0;

		for (int j=0; j<primitives_count; j++)
		{
			Triangle* T = dynamic_cast<Triangle*>( (*primitives_ptr)[primitives_start + j] );
			if (center(T).x < x_divide) {
				left_cnt++;
			} else {
				right_cnt++;
			}
		}
		bool accepted = false;
		left_tmp->primitives_start = primitives_start;
		left_tmp->primitives_count = left_cnt;
		right_tmp->primitives_start = primitives_start + left_cnt;
		right_tmp->primitives_count = right_cnt;
		std::sort(primitives_ptr->begin() + primitives_start, primitives_ptr->begin() + primitives_start + primitives_count,
		[center](Primitive* P1, Primitive* P2){
			float x1 = center(P1).x;
			float x2 = center(P2).x;
			return x1 < x2;
		});
		left_tmp->update_extents();
		right_tmp->update_extents();

		if (left_cnt > 0 && right_cnt > 0) {
			float SA = left_tmp->surface_area() + right_tmp->surface_area();
			if (SA < min_SA) {
				left = left_tmp;
				right = right_tmp;
				min_SA = SA;
				min_divide_axis = 0;
				accepted = true;
			}
		}

		if (!accepted) {
			delete left_tmp;
			delete right_tmp;
		}
	}
	// y axis
	for (int i=1; i<PER_AXIS_GRANULARITY; i++)
	{
		float y_divide = min.y + i * step.y;
		BVH* left_tmp = new BVH(primitives_ptr);
		BVH* right_tmp = new BVH(primitives_ptr);
		uint left_cnt = 0, right_cnt = 0;

		for (int j=0; j<primitives_count; j++)
		{
			Triangle* T = dynamic_cast<Triangle*>( (*primitives_ptr)[primitives_start + j] );
			if (center(T).y < y_divide) {
				left_cnt++;
			} else {
				right_cnt++;
			}
		}
		bool accepted = false;
		left_tmp->primitives_start = primitives_start;
		left_tmp->primitives_count = left_cnt;
		right_tmp->primitives_start = primitives_start + left_cnt;
		right_tmp->primitives_count = right_cnt;
		std::sort(primitives_ptr->begin() + primitives_start, primitives_ptr->begin() + primitives_start + primitives_count,
		[center](Primitive* P1, Primitive* P2){
			float x1 = center(P1).y;
			float x2 = center(P2).y;
			return x1 < x2;
		});
		left_tmp->update_extents();
		right_tmp->update_extents();

		if (left_cnt > 0 && right_cnt > 0) {
			float SA = left_tmp->surface_area() + right_tmp->surface_area();
			if (SA < min_SA) {
				left = left_tmp;
				right = right_tmp;
				min_SA = SA;
				min_divide_axis = 1;
				accepted = true;
			}
		}

		if (!accepted) {
			delete left_tmp;
			delete right_tmp;
		}
	}
	// x axis
	for (int i=1; i<PER_AXIS_GRANULARITY; i++)
	{
		float z_divide = min.z + i * step.z;
		BVH* left_tmp = new BVH(primitives_ptr);
		BVH* right_tmp = new BVH(primitives_ptr);
		uint left_cnt = 0, right_cnt = 0;

		for (int j=0; j<primitives_count; j++)
		{
			Triangle* T = dynamic_cast<Triangle*>( (*primitives_ptr)[primitives_start + j] );
			if (center(T).z < z_divide) {
				left_cnt++;
			} else {
				right_cnt++;
			}
		}
		bool accepted = false;
		left_tmp->primitives_start = primitives_start;
		left_tmp->primitives_count = left_cnt;
		right_tmp->primitives_start = primitives_start + left_cnt;
		right_tmp->primitives_count = right_cnt;
		std::sort(primitives_ptr->begin() + primitives_start, primitives_ptr->begin() + primitives_start + primitives_count,
		[center](Primitive* P1, Primitive* P2){
			float x1 = center(P1).z;
			float x2 = center(P2).z;
			return x1 < x2;
		});
		left_tmp->update_extents();
		right_tmp->update_extents();

		if (left_cnt > 0 && right_cnt > 0) {
			float SA = left_tmp->surface_area() + right_tmp->surface_area();
			if (SA < min_SA) {
				left = left_tmp;
				right = right_tmp;
				min_SA = SA;
				min_divide_axis = 2;
				accepted = true;
			}
		}

		if (!accepted) {
			delete left_tmp;
			delete right_tmp;
		}
	}

	if (min_divide_axis==0) {
		std::sort(primitives_ptr->begin() + primitives_start, primitives_ptr->begin() + primitives_start + primitives_count,
		[center](Primitive* P1, Primitive* P2){
			float x1 = center(P1).x;
			float x2 = center(P2).x;
			return x1 < x2;
		});
	} else if (min_divide_axis==1) {
		std::sort(primitives_ptr->begin() + primitives_start, primitives_ptr->begin() + primitives_start + primitives_count,
		[center](Primitive* P1, Primitive* P2){
			float x1 = center(P1).y;
			float x2 = center(P2).y;
			return x1 < x2;
		});
	} else {
		std::sort(primitives_ptr->begin() + primitives_start, primitives_ptr->begin() + primitives_start + primitives_count,
		[center](Primitive* P1, Primitive* P2){
			float x1 = center(P1).z;
			float x2 = center(P2).z;
			return x1 < x2;
		});
	}

	// need this check because there could be cases when all divisions result in one child being empty.
	// In that case make this node a leaf.
	if (left && right) {
		LOGF("left %u [%u, %u), right %u [%u, %u)", 
			left->primitives_count, left->primitives_start, left->primitives_start + left->primitives_count, 
			right->primitives_count, right->primitives_start, right->primitives_start + right->primitives_count);
	} else {
		LOG("(automatical division)");
		left = new BVH(primitives_ptr);
		right = new BVH(primitives_ptr);
		left->primitives_start = primitives_start;
		left->primitives_count = primitives_count / 2;
		left->update_extents();
		right->primitives_start = primitives_start + left->primitives_count;
		right->primitives_count = primitives_count - left->primitives_count;
		right->update_extents();
	}
	left->expand_bvh();
	right->expand_bvh();
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
	Primitive* primitive = nullptr;

#if 0
	for (size_t i = 0; i < primitives_count; i++) {
		Primitive* prim_tmp = (*primitives_ptr)[primitives_start + i]->intersect(ray, t, n, true);
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
			for (size_t i = 0; i < primitives_count; i++) {
				Primitive* prim_tmp = (*primitives_ptr)[primitives_start + i]->intersect(ray, t, n, true);
				if (prim_tmp) {
					primitive = prim_tmp;
				}
			}
		}
	}
#endif
	return primitive;
}