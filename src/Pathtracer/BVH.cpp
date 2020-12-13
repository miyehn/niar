#include "BVH.hpp"

#define BVH_THRESHOLD 16
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

	// sort them back (since primitive order might've been modified along the way)
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
bool BVH::intersect_aabb(const Ray& ray, float& tmin, float& tmax)
{
#if 0 // use my own (readable but a tad slower) impl?
	float dmin = INF;
	bool hit = false;

	float txmin = (min.x - ray.o.x) / ray.d.x;
	float txmax = (max.x - ray.o.x) / ray.d.x;
	float tymin = (min.y - ray.o.y) / ray.d.y;
	float tymax = (max.y - ray.o.y) / ray.d.y;
	float tzmin = (min.z - ray.o.z) / ray.d.z;
	float tzmax = (max.z - ray.o.z) / ray.d.z;

	if (!isnan(txmin) && txmin >= ray.tmin && txmin < ray.tmax) {
		//float px = ray.o.x + txmin*ray.d.x;
		float py = ray.o.y + txmin*ray.d.y;
		float pz = ray.o.z + txmin*ray.d.z;
		if (py >= min.y && py < max.y &&
			pz >= min.z && pz < max.z)
		{
			dmin = glm::min(dmin, txmin);
			hit = true;
		}
	}

	if (!isnan(txmax) && txmax >= ray.tmin && txmax < ray.tmax) {
		//float px = ray.o.x + txmax*ray.d.x;
		float py = ray.o.y + txmax*ray.d.y;
		float pz = ray.o.z + txmax*ray.d.z;
		if (py >= min.y && py < max.y &&
			pz >= min.z && pz < max.z)
		{
			dmin = glm::min(dmin, txmax);
			hit = true;
		}
	}

	if (!isnan(tymin) && tymin >= ray.tmin && tymin < ray.tmax) {
		float px = ray.o.x + tymin*ray.d.x;
		//float py = ray.o.y + tymin*ray.d.y;
		float pz = ray.o.z + tymin*ray.d.z;
		if (px >= min.x && px < max.x &&
			pz >= min.z && pz < max.z)
		{
			dmin = glm::min(dmin, tymin);
			hit = true;
		}
	}

	if (!isnan(tymax) && tymax >= ray.tmin && tymax < ray.tmax) {
		float px = ray.o.x + tymax*ray.d.x;
		//float py = ray.o.y + tymax*ray.d.y;
		float pz = ray.o.z + tymax*ray.d.z;
		if (px >= min.x && px < max.x &&
			pz >= min.z && pz < max.z)
		{
			dmin = glm::min(dmin, tymax);
			hit = true;
		}
	}

	if (!isnan(tzmin) && tzmin >= ray.tmin && tzmin < ray.tmax) {
		float px = ray.o.x + tzmin*ray.d.x;
		float py = ray.o.y + tzmin*ray.d.y;
		//float pz = ray.o.z + tzmin*ray.d.z;
		if (px >= min.x && px < max.x &&
			py >= min.y && py < max.y)
		{
			dmin = glm::min(dmin, tzmin);
			hit = true;
		}
	}

	if (!isnan(tzmax) && tzmax >= ray.tmin && tzmax < ray.tmax) {
		float px = ray.o.x + tzmax*ray.d.x;
		float py = ray.o.y + tzmax*ray.d.y;
		//float pz = ray.o.z + tzmax*ray.d.z;
		if (px >= min.x && px < max.x &&
			py >= min.y && py < max.y)
		{
			dmin = glm::min(dmin, tzmax);
			hit = true;
		}
	}

	if (hit) {
		distance = dmin;
		return true;
	} else {
		return false;
	}

#else
	tmin = (min.x - ray.o.x) / ray.d.x; 
	tmax = (max.x - ray.o.x) / ray.d.x; 
 
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

	return tmax >= ray.tmin; 
#endif
	
}

#define USE_BVH 1
#define FRONT_TO_BACK 0

Primitive* BVH::intersect_primitives(Ray& ray, double& t, vec3& n) 
{
	Primitive* primitive = nullptr;

#if USE_BVH

#if FRONT_TO_BACK

	if (left || right)
	{
		float tmin_left, tmax_left, tmin_right, tmax_right;
		if (left->intersect_aabb(ray, tmin_left, tmax_right))
		{
			if (right->intersect_aabb(ray, tmin_right, tmax_right)) // intersected both bboxes, go with the closer one
			{
				BVH* first = nullptr;
				BVH* second = nullptr;
				float tmin_near, tmin_far;
				if (tmin_left < tmin_right) {
					first = left; second = right;
					tmin_near = tmin_left; tmin_far = tmin_right;
				} else {
					first = right; second = left;
					tmin_near = tmin_right; tmin_far = tmin_left;
				}

				primitive = first->intersect_primitives(ray, t, n);
				if (!primitive || t > tmin_far) {
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
#else
	float tmin, tmax;
	if (intersect_aabb(ray, tmin, tmax))
	{
		if (left || right)
		{
			primitive = left->intersect_primitives(ray, t, n);
			Primitive* prim_tmp = right->intersect_primitives(ray, t, n);
			if (prim_tmp) primitive = prim_tmp;
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

#endif // FRONT_TO_BACK

#else
	for (size_t i = 0; i < primitives_count; i++) {
		Primitive* prim_tmp = (*primitives_ptr)[primitives_start + i]->intersect(ray, t, n, true);
		if (prim_tmp) {
			primitive = prim_tmp;
		}
	}
#endif // USE_BVH
	return primitive;
}