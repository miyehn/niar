#include "pathtracer_kernel_utils.ispc"

inline bool intersect(Triangle triangle, Ray& ray, float& t, vec3& normal, bool modify_ray) {
	// ray parallel to plane
	float d_dot_n = dot(ray.d, triangle.plane_n);
	if (abs(d_dot_n) == 0.0f) return false;
	// intesection out of range
	float _t = (triangle.plane_k - dot(ray.o, triangle.plane_n)) / d_dot_n;
	if (_t < ray.tmin || _t > ray.tmax) return false;

	vec3 p = vadd( ray.o, smul( (float)_t, ray.d ) );
	// barycentric coordinate with axes v[1] - v[0], v[2] - v[0]
	vec3 p0 = vsub( p, triangle.vertices[0] );

	// other early outs: intersection not in triangle
	if (dot( vsub(p, triangle.vertices[0]), triangle.enormals[0]) < 0) return false;
	if (dot( vsub(p, triangle.vertices[1]), triangle.enormals[1]) < 0) return false;
	if (dot( vsub(p, triangle.vertices[2]), triangle.enormals[2]) < 0) return false;

	// intersection is valid
	if (modify_ray) ray.tmax = _t;
	t = _t;
	normal = triangle.plane_n;

	return true;
}

inline bool intersect_aabb(int bvh_index, Ray& ray)
{
	BVH* bvh = G.bvh_root + bvh_index;
	vec3 min = bvh->min;
	vec3 max = bvh->max;

	float tmin = (min.x - ray.o.x) / ray.d.x; 
	float tmax = (max.x - ray.o.x) / ray.d.x; 
 
	if (tmin > tmax) {
		float tmp = tmin; tmin = tmax; tmax = tmp;
	}
 
	float tymin = (min.y - ray.o.y) / ray.d.y; 
	float tymax = (max.y - ray.o.y) / ray.d.y; 
 
	if (tymin > tymax) {
		float tmp = tymin; tymin = tymax; tymax = tmp;
	}
 
	if ((tmin > tymax) || (tymin > tmax)) 
		return false; 
 
	if (tymin > tmin) tmin = tymin;
	if (tymax < tmax) tmax = tymax;
 
	float tzmin = (min.z - ray.o.z) / ray.d.z; 
	float tzmax = (max.z - ray.o.z) / ray.d.z; 
 
	if (tzmin > tzmax) {
		float tmp = tzmin; tzmin = tzmax; tzmax = tmp;
	}
 
	if ((tmin > tzmax) || (tzmin > tmax)) 
		return false; 
 
	if (tzmin > tmin) 
		tmin = tzmin; 
 
	if (tzmax < tmax) 
		tmax = tzmax; 

	// if (isnan(tmin) || isnan(tmax)) return false;

	return tmax >= ray.tmin && tmin < ray.tmax;
}

inline int intersect_bvh_triangles(int bvh_index, Ray& ray, float& t, vec3& normal)
{
	#if 0
	int triangle_index = -1;

	uint* st = new uint[G.bvh_stack_size];
	uint top = 0;
	st[top] = 0; top++; // push

	uint max_top = 0;

	while (top > 0)
	{
		uint bvh_idx = st[top-1]; top--; // pop
		if (intersect_aabb(bvh_idx, ray))
		{
			BVH* bvh = G.bvh_root + bvh_idx;
			int l = bvh->left_index, r = bvh->right_index;
			if (l >= 0 && r >= 0)
			{
				st[top] = r; top++; // push
				st[top] = l; top++; // push
			}
			else
			{
				for (uint i=bvh->triangles_start; i<bvh->triangles_start+bvh->triangles_count; i++) {
					Triangle* T = G.triangles + i;
					if (intersect(*T, ray, t, normal, true)) {
						triangle_index = i;
					}
				}
			}
		}
		if (top > max_top) max_top = top;
	}
	delete[] st;

	return triangle_index;

	#else
	int triangle_index = -1;
	if (intersect_aabb(bvh_index, ray))
	{
		BVH* bvh = G.bvh_root + bvh_index;
		int l = bvh->left_index, r = bvh->right_index;
		if (l>=0 && r>=0)
		{
			triangle_index = intersect_bvh_triangles(l, ray, t, normal);
			int tmp = intersect_bvh_triangles(r, ray, t, normal);
			if (tmp >= 0) triangle_index = tmp;
		}
		else
		{
			for (uint i=bvh->triangles_start; i < bvh->triangles_start + bvh->triangles_count; i++) {
				Triangle* T = G.triangles + i;
				if (intersect(*T, ray, t, normal, true)) {
					triangle_index = i;
				}
			}
		}
	}
	return triangle_index;
	#endif
}

inline int intersect_scene(Ray& ray, float& t, vec3& normal)
{
	if (G.use_bvh)
	{
		return intersect_bvh_triangles(0, ray, t, normal);
	}
	else
	{
		int triangle_index = -1;
		for (uint i = 0; i < G.num_triangles; i++) {
			if (intersect(G.triangles[i], ray, t, normal, true)) {
				triangle_index = i;
			}
		}
		return triangle_index;
	}
}


//-------- lights --------

inline float ray_to_light_pdf(Ray& ray, vec3& origin, Triangle* triangle)
{
	vec3 light_p = sample_point_in_triangle(triangle);

	ray.o = origin;
	ray.d = normalized( vsub(light_p, origin) );
	float t; vec3 n;
	intersect(*triangle, ray, t, n, true);

	float d2 = t * t;

	float costheta_l = max(0.0f, dot( neg(ray.d), n ));

	float eps_adjusted = EPSILON / costheta_l;
	//ray.o = vadd(ray.o, smul(10 * eps_adjusted, ray.d));
	ray.tmin = eps_adjusted;
	ray.tmax -= eps_adjusted;

	return d2 / (triangle->area * costheta_l);
}

//-------- materials (BSDF) --------

inline vec3 BSDF_f(BSDF* bsdf, vec3& wi, vec3& wo)
{
	if (bsdf->type == Mirror)
	{
		return new_vec3();
	}
	else if (bsdf->type == Glass)
	{
		return new_vec3();
	}
	else
	{
		return smul(ONE_OVER_PI, bsdf->albedo);
	}
}

#define IOR 1.52f

inline vec3 BSDF_sample_f(BSDF* bsdf, float& pdf, vec3& wi, vec3 wo)
{
	if (bsdf->type == Mirror)
	{
		wi = neg(wo);
		wi.z = wo.z;
		pdf = 1.0f;
		return smul(1.0f / wi.z, bsdf->albedo);
	}
	else if (bsdf->type == Glass)
	{
		// will treat wo as in direction and wi as out direction, since it's bidirectional

		bool trace_out = wo.z < 0; // the direction we're going to trace is into the medium

		// IOR, assume container medium is air
		float ni = trace_out ? IOR : 1.0f;
		float nt = trace_out ? 1.0f : IOR;

		float cos_theta_i = abs(wo.z);
		float sin_theta_i = sqrt(1.0f - pow(cos_theta_i, 2));

		// opt out early for total internal reflection
		float cos_sq_theta_t = 1.0f - pow(ni/nt, 2) * (1.0f - wo.z * wo.z);
		bool TIR = cos_sq_theta_t < 0;
		if (TIR) { // total internal reflection
			wi = neg(wo);
			wi.z = wo.z;
			pdf = 1.0f;
			return smul( 1.0f / abs(wi.z), bsdf->albedo );
		}
		
		// then use angles to find reflectance
		float r0 = pow((ni-nt) / (ni+nt), 2);
		float reflectance = r0 + (1.0f - r0) * pow(1.0f - cos_theta_i, 5);
		
		// flip a biased coin to decide whether to reflect or refract
		bool reflect = rand01() <= reflectance;
		if (reflect) {
			wi = neg(wo);
			wi.z = wo.z;
			pdf = reflectance;
			return smul( reflectance / cos_theta_i, bsdf->albedo );

		} else { // refract
			// remember we treat wi as "out direction"
			float cos_theta_t = sqrt(cos_sq_theta_t);
			float sin_theta_t = sqrt(1.0f - cos_sq_theta_t);

			float xy_norm_factor = sin_theta_t / sin_theta_i;
			wi = new_vec3(-wo.x * xy_norm_factor,
								-wo.y * xy_norm_factor,
								trace_out ? cos_theta_t : -cos_theta_t);

			pdf = 1.0f - reflectance;
			// now compute f...
			return smul( ((nt*nt) / (ni*ni)) * (1.0f - reflectance) * (1.0f / cos_theta_i), bsdf->albedo );
		}
	}
	else
	{
		wi = sample_hemisphere_cos_weighed();
		pdf = wi.z * ONE_OVER_PI;
		return smul(ONE_OVER_PI, bsdf->albedo);
	}
}

//-------- actual path tracing --------

inline void make_h2w(mat3& h2w, vec3& z) {
	vec3 tmp; tmp.x = 1.0f; tmp.y = 2.0f, tmp.z = 3.0f;
	tmp = normalized(tmp);

	vec3 x = cross(tmp, z);
	x = normalized(x);
	vec3 y = cross(z, x);

	h2w.colx = x;
	h2w.coly = y;
	h2w.colz = z;
}

inline void set_color(uint i, vec3 &color) {
	uint i3 = i * 3;
	color = clamp01(color);
	G.output[i3] = 0xFF & (int)(color.x * 255.0f);
	G.output[i3 + 1] = 0xFF & (int)(color.y * 255.0f);
	G.output[i3 + 2] = 0xFF & (int)(color.z * 255.0f);
}

inline void generate_ray(RayTask* rtask, uint pixel_index, uint ray_index)
{
	*rtask = new_RayTask();
	rtask->finished = false;
	int w, h;
	if (G.rendering_to_tile) {
		w = G.tile_size * G.tile_indexX + pixel_index % G.tile_width;
		h = G.tile_size * G.tile_indexY + pixel_index / G.tile_width;
	} else {
		w = pixel_index % G.width;
		h = pixel_index / G.width;
	}
	float fov = G.camera->fov;
	float half_width = G.width / 2.0f;
	float half_height = G.height / 2.0f;

	float k_y = tan(fov / 2.0f);
	float k_x = k_y * G.camera->aspect_ratio;

	Ray* ray = &(rtask->ray);

	////////
	float offset_x = G.pixel_offsets[2 * ray_index];
	float offset_y = G.pixel_offsets[2 * ray_index + 1];

	ray->o = G.camera->position;
	ray->tmin = 0.0f;
	ray->tmax = 999999999.0f;

	float dx = (w + offset_x - half_width) / half_width;
	float dy = (h + offset_y - half_height) / half_height;

	vec3 d_unnormalized_c = new_vec3(k_x * dx, k_y * dy, -1.0);
	varying mat3 c2wr = G.camera->camera_to_world_rotation;
	vec3 d_unnormalized_w = mmul(c2wr, d_unnormalized_c);
	ray->d = normalized(d_unnormalized_w);

	if (G.use_dof) {
		vec3 focal_p = vadd(ray->o, smul(G.focal_distance, d_unnormalized_w));
		vec3 aperture_shift_cam = smul(G.aperture_radius, sample_unit_disc());
		vec3 aperture_shift_world = mmul(c2wr, aperture_shift_cam);
		ray->o = vadd(G.camera->position, aperture_shift_world);
		ray->d = normalized(vsub(focal_p, ray->o));
	}
}

inline BSDF_t get_bsdf_t(RayTask* ray_tasks, uint rtask_offset) {
	uint triangle_index = ray_tasks[rtask_offset].triangle_index;
	uint bsdf_index = G.triangles[ triangle_index ].bsdf_index;
	return G.bsdfs[ bsdf_index ].type;
}

// counting sort: https://brilliant.org/wiki/counting-sort/
inline void count_rays_bsdf(uniform uint bsdf_buckets[], uniform uint tasks_offset, uniform uint num_tasks)
{
	uniform RayTask* ray_tasks_in = G.ray_tasks_backup + tasks_offset;
	uniform RayTask* ray_tasks_out = G.ray_tasks + tasks_offset;

	// count how many rays does each material have
	varying uint bsdf_buckets_partial[NUM_MATERIAL_TYPES];
	foreach (prog = 0 ... programCount) {
		// initialize the array for all program instances (one array each)
		for (int i = 0; i < NUM_MATERIAL_TYPES; i++) {
			bsdf_buckets_partial[i] = 0;
		}
	}
	foreach (i = 0 ... num_tasks) {
		uint bsdf_type = (uint)get_bsdf_t(ray_tasks_in, i);
		bsdf_buckets_partial[bsdf_type]++;
	}
	for (uniform uint cur_bsdf = 0; cur_bsdf < NUM_MATERIAL_TYPES; cur_bsdf++) {
		bsdf_buckets[cur_bsdf] = reduce_add(bsdf_buckets_partial[cur_bsdf]);
	}
	scan_excl(bsdf_buckets, NUM_MATERIAL_TYPES);
}

inline void sort_rays_bsdf(uniform uint tasks_offset, uniform uint num_tasks)
{
	uniform RayTask* ray_tasks_in = G.ray_tasks_backup + tasks_offset;
	uniform RayTask* ray_tasks_out = G.ray_tasks + tasks_offset;

	uniform uint indices[SCAN_SIZE];
	uniform uint old_idx = 0;
	for (uniform uint cur_bsdf = 0; cur_bsdf < NUM_MATERIAL_TYPES; cur_bsdf++) {

		foreach (i = 0 ... num_tasks) {
			uint bsdf_type = (uint)get_bsdf_t(ray_tasks_in, i);
			indices[i] = (bsdf_type == cur_bsdf);
		}
		scan_excl(indices, num_tasks);
		uniform uint idx = indices[num_tasks-1];
		uint last_bsdf = (uint)get_bsdf_t(ray_tasks_in, num_tasks-1);
		if (last_bsdf == cur_bsdf) {
			idx += 1;
		}
		if (idx == 0) {
			continue;
		}
		// reorder
		uniform RayTask* ray_tasks_out_local = ray_tasks_out + old_idx;
		foreach (i = 0 ... num_tasks-1) {
			if (indices[i] != indices[i+1]) {
				ray_tasks_out_local[indices[i]] = *(ray_tasks_in + i);
			}
		}
		if (last_bsdf == cur_bsdf) {
			ray_tasks_out_local[ indices[num_tasks-1] ] = *(ray_tasks_in + num_tasks - 1);
		}
		old_idx = old_idx+idx;
	}
}

// only traces one bounce (and set depth and finished)
// also updates G.output_tmp along the way
inline void trace_ray(RayTask &rtask)
{
	if (rtask.depth >= G.max_ray_depth) {
		rtask.finished = true;
	}
	if (rtask.finished) return;

	Ray &ray = rtask.ray;

	// info of closest hit
	Triangle* primitive = G.triangles + rtask.triangle_index;
	BSDF* bsdf = G.bsdfs + primitive->bsdf_index;
	float t = ray.tmax;
	vec3 n = rtask.triangle_normal;

	// pre-compute (or declare) some common things to be used later
	vec3* L = G.output_tmp + rtask.buf_index;
	vec3 hit_p = vadd(ray.o, smul(t, ray.d));
	// construct transform from hemisphere space to world space
	mat3 h2w;
	make_h2w(h2w, n);
	mat3 w2h = transpose(h2w);
	// wi, wo
	vec3 wo_world = neg(ray.d);
	vec3 wo_hemi = neg( mmul(w2h, ray.d) );
	//
	vec3 wi_world; // to be transformed from wi_hemi
	vec3 wi_hemi; // to be assigned by f
	float costhetai; // some variation of dot(wi_world, n)

	//---- emission ----

	if (G.use_direct_light) {
		if (rtask.depth == 0 || ray.receive_le || !bsdf->is_emissive) {
			*L = vadd(*L, vmul(rtask.contribution, bsdf->Le));
		}
	} else {
		*L = vadd(*L, vmul(rtask.contribution, bsdf->Le));
	}

	//---- direct lighting ----

	if (G.use_direct_light && !bsdf->is_delta)
	{
		for (uint i = 0; i < G.num_area_lights; i++)
		{
			Triangle* area_light_T = G.triangles + G.area_light_indices[i];
			for (uint j = 0; j < G.area_light_samples; j++)
			{
				// get ray to light
				Ray ray_to_light = new_Ray();
				// in this case not a real pdf, but just something to divide by?
				float pdf = ray_to_light_pdf(ray_to_light, hit_p, area_light_T);

				// test if ray to light hits anything other than the starting primitive and the light
				float tmp_t; vec3 tmp_n;
				bool in_shadow = intersect_scene(ray_to_light, tmp_t, tmp_n) >= 0;

				// add contribution
				if (!in_shadow) {
					wi_world = ray_to_light.d;
					wi_hemi = mmul(w2h, wi_world);

					costhetai = max(0.0f, dot(n, wi_world));
					vec3 L_direct = vmul(
						G.bsdfs[area_light_T->bsdf_index].Le,
						smul( costhetai / pdf , BSDF_f(bsdf, wi_hemi, wo_hemi) ));
					// correction for when above num and denom both 0 (see CPP)
					if (isnan(L_direct.x) || isnan(L_direct.y) || isnan(L_direct.z)) L_direct = new_vec3();
					*L = vadd( *L, vmul(rtask.contribution, smul(G.area_light_sample_weight, L_direct)) );
				}
			}
		}
	}

	#if 0
	rtask.finished = true;
	#else
	//---- indirect lighting ----

	if ( (G.use_direct_light && !bsdf->is_emissive) ||
		 !G.use_direct_light )
	{
		float pdf;
		vec3 f = BSDF_sample_f(bsdf, pdf, wi_hemi, wo_hemi);

		// transform wi back to world space
		wi_world = mmul(h2w, wi_hemi);
		costhetai = abs(dot(n, wi_world));

		// russian roulette
		float termination_prob = 0.0f;
		float rr_contribution = brightness(rtask.contribution) * brightness(f) * costhetai;
		if (rr_contribution < G.rr_threshold) {
			termination_prob = (G.rr_threshold - rr_contribution) / G.rr_threshold;
		}
		bool terminate = rand01() < termination_prob;

		// recursive step: trace scattered ray in wi direction (if not terminated by RR)
		if (!terminate) {
			vec3 refl_offset = wi_hemi.z > 0 ? smul(EPSILON, n) : neg( smul(EPSILON, n) );
			ray.o = vadd(hit_p, refl_offset);
			ray.d = wi_world;
			ray.tmax = 999999999.0f;
			ray.receive_le = G.use_direct_light && bsdf->is_delta;
			
			float tt;
			int next_triangle = intersect_scene(ray, tt, rtask.triangle_normal);
			if (next_triangle >= 0) {
				rtask.triangle_index = next_triangle;
				rtask.contribution = vmul(rtask.contribution,
					smul(costhetai / pdf * (1.0f / (1.0f - termination_prob)), f));
			} else {
				rtask.finished = true;
			}
		} else {
			rtask.finished = true;
		}
	}
	#endif
	rtask.depth += 1;
	
}

// for ray tasks in range, check ray_tmp's intersection with the scene and store result in ray_tmp_hit_primitive.
inline void intersect_bvh_all(uniform uint start, uniform uint count, RTaskBuffer buf)
{
	// TODO.
	foreach (i = start ... start + count)
	{
		RayTask* rtask = (buf==RTaskBufferA ? G.ray_tasks : G.ray_tasks_backup) + i;
		Ray rtmp = rtask->ray_tmp;
		float tmp_t; vec3 tmp_n;
		rtask->ray_tmp_hit_primitive = intersect_scene(rtmp, tmp_t, tmp_n);
		rtask->v0_buf = tmp_n;
		rtask->ray_tmp = rtmp;
	}
}

// trace these rays for one bounce
inline void trace_rays(uniform uint start, uniform uint count)
{
	#if 1
	foreach (i = 0 ... count)
	{
		RayTask* rtask_ptr = G.ray_tasks + start + i;
		RayTask rtask = *(rtask_ptr);
		trace_ray(rtask);
		*rtask_ptr = rtask;
	}
	#else

	RayTask* rtasks = G.ray_tasks + start;

	//-------- emission --------
	if (G.use_direct_light)
	{
		foreach (i = 0 ... count)
		{
			RayTask* rtask = rtasks + i;
			Triangle* primitive = G.triangles + rtask->triangle_index;
			BSDF* bsdf = G.bsdfs + primitive->bsdf_index;
			vec3* L = G.output_tmp + rtask->buf_index;
			if ( rtask->depth==0 || rtask->ray.receive_le || !bsdf->is_emissive) {
				*L = vadd(*L, vmul(rtask->contribution, bsdf->Le));
			}
		}
	}
	else
	{
		foreach (i = 0 ... count)
		{
			RayTask* rtask = rtasks + i;
			Triangle* primitive = G.triangles + rtask->triangle_index;
			BSDF* bsdf = G.bsdfs + primitive->bsdf_index;
			vec3* L = G.output_tmp + rtask->buf_index;
			*L = vadd(*L, vmul(rtask->contribution, bsdf->Le));
		}
	}

	//---------------------------------

	uniform uint num_not_deltas = count;

	// gather rays based on delta-ness
	// NotDelta?
	gather_rays(start, num_not_deltas, true, Nothing); // now working set becomes ray_task_backup
	rtasks = G.ray_tasks_backup + start;

	//-------- direct lighting --------
	for (uniform uint i = 0; i < G.num_area_lights; i++) // each light 
	{
		Triangle* area_light_T = G.triangles + G.area_light_indices[i];
		for (uniform uint j = 0; j < G.area_light_samples; j++) // each sample (1)
		{
			// generate direct light ray sample
			foreach (r = 0 ... num_not_deltas)
			{
				RayTask* rtask = rtasks + r;
				vec3 hit_p = vadd(rtask->ray.o, smul(rtask->ray.tmax, rtask->ray.d));
				// get ray to light
				Ray ray_to_light = new_Ray();
				// in this case not a real pdf, but just something to divide by?
				rtask->s0_buf = ray_to_light_pdf(ray_to_light, hit_p, area_light_T);
				rtask->ray_tmp = ray_to_light;
				rtask->ray_tmp_hit_primitive = -1;
			}

			// test all those intersections
			intersect_bvh_all(start, num_not_deltas, RTaskBufferB);

			// calculate and add direct light contribution
			foreach (r = 0 ... num_not_deltas)
			{
				RayTask* rtask = rtasks + r;
				if (rtask->ray_tmp_hit_primitive < 0) // direct light ray didn't hit anything; not in shadow
				{ // hit_p is not in shadow of this light source -> add contribution
					vec3 d = rtask->ray.d;
					Triangle* primitive = G.triangles + rtask->triangle_index;
					BSDF* bsdf = G.bsdfs + primitive->bsdf_index;
					
					vec3 n = rtask->triangle_normal;
					mat3 h2w;
					make_h2w(h2w, n);
					mat3 w2h = transpose(h2w);
					// wi, wo
					vec3 wi_world = rtask->ray_tmp.d;
					vec3 wi_hemi = mmul(w2h, wi_world);
					vec3 wo_world = neg(d);
					vec3 wo_hemi = neg( mmul(w2h, d) );

					float costhetai = max(0.0f, dot(n, wi_world));
					vec3 L_direct = vmul(
						G.bsdfs[area_light_T->bsdf_index].Le,
						smul( costhetai / rtask->s0_buf, BSDF_f(bsdf, wi_hemi, wo_hemi) ));
					if (isnan(L_direct.x) || isnan(L_direct.y) || isnan(L_direct.z)) L_direct = new_vec3();
					vec3* L = G.output_tmp + rtask->buf_index;
					*L = vadd( *L, vmul(rtask->contribution, smul(G.area_light_sample_weight, L_direct)) );
				}
			}
		}
	}

	//-------- indirect lighting --------

	// mark rays that need to compute indirect lighting
	foreach (i = 0 ... count)
	{
		RayTask* rtask = rtasks + i;
		Triangle* primitive = G.triangles + rtask->triangle_index;
		BSDF* bsdf = G.bsdfs + primitive->bsdf_index;
		rtask->s0_buf = 0;
		rtask->ray_tmp_hit_primitive = -1;
		if ( !rtask->finished && ((G.use_direct_light && !bsdf->is_emissive) || !G.use_direct_light) )
		{
			vec3 n = rtask->triangle_normal;
			mat3 h2w;
			make_h2w(h2w, n);
			mat3 w2h = transpose(h2w);

			vec3 wo_hemi = neg( mmul(w2h, rtask->ray.d) );
			float pdf; vec3 wi_hemi;
			vec3 f = BSDF_sample_f(bsdf, pdf, wi_hemi, wo_hemi);
			vec3 wi_world = mmul( transpose(w2h), wi_hemi );
			float costhetai = abs( dot(rtask->triangle_normal, wi_world) );

			rtask->v0_buf = wi_hemi;

			// russian roulette
			float termination_prob = 0.0f;
			float rr_contribution = brightness(rtask->contribution) * brightness(f) * costhetai;
			if (rr_contribution < G.rr_threshold) {
				termination_prob = (G.rr_threshold - rr_contribution) / G.rr_threshold;
			}
			bool terminate = rand01() < termination_prob;

			if (!terminate) {

				rtask->s0_buf = 1;

				// TODO: move out
				Ray rtmp;
				vec3 refl_offset = wi_hemi.z > 0 ? smul(EPSILON, n) : neg( smul(EPSILON, n) );
				vec3 hit_p = vadd(rtask->ray.o, smul(rtask->ray.tmax, rtask->ray.d));
				rtmp.o = vadd(hit_p, refl_offset);
				rtmp.d = wi_world;
				rtmp.tmin = 0.0f;
				rtmp.tmax = 999999999.0f;
				rtmp.receive_le = G.use_direct_light && bsdf->is_delta;
				
				rtask->ray_tmp = rtmp;
				rtask->v1_buf = smul(costhetai / pdf * (1.0f / (1.0f - termination_prob)), f);

			} else {
				rtask->finished = true;
			}
		}
	}

	// gather rays that need to compute indirect lighting
	uniform uint num_indirect_rays = count;
	// Unfinished?
	gather_rays(start, num_indirect_rays, false, Nothing); // now working set becomes ray_task_backup
	rtasks = G.ray_tasks + start;

	intersect_bvh_all(start, count, RTaskBufferA);

	// compute indirect lighting
	foreach (i = 0 ... count)
	{
		RayTask* rtask = rtasks + i;
		rtask->ray = rtask->ray_tmp;
		rtask->triangle_normal = rtask->v0_buf;
		if (rtask->ray_tmp_hit_primitive >= 0)
		{
			rtask->triangle_index = rtask->ray_tmp_hit_primitive;
			rtask->contribution = vmul(rtask->contribution, rtask->v1_buf);
		} else {
			rtask->finished = true;
		}
	}

	///////////////////////////////////

	foreach (i = 0 ... count) {
		RayTask* rtask = rtasks + i;
		rtask->depth += 1;
	}

	#endif
}

/*
task void raytrace_scene_ispc_task(
	uniform uint batch,
	uniform uint pixels_per_mini_batch,
	uniform uint pixels_per_batch,
	uniform uint buf_size,
	uniform float ray_weight) 
{
	uniform uint minibatch = taskIndex;
	uniform uint max_num_pixels = pixels_per_mini_batch;
	if ((minibatch + 1)*pixels_per_mini_batch > pixels_per_batch) {
		max_num_pixels = (minibatch + 1)*pixels_per_mini_batch - pixels_per_batch;
	}

	//-------- generate the initial raytasks --------
	foreach (pixel_index_mini = 0 ... max_num_pixels, ray_index = 0 ... G.num_offsets)
	{
		uint pixel_index_local = pixel_index_mini + pixels_per_mini_batch * minibatch;
		uint pixel_index = pixel_index_local + batch * pixels_per_batch;
		if (pixel_index < buf_size)
		{
			uint ray_index_batch = pixel_index_local * G.num_offsets + ray_index;
			RayTask* rtask_ptr = G.ray_tasks + ray_index_batch;
			generate_ray(rtask_ptr, pixel_index, ray_index);

			// pre-calculate the first hit
			float tt; vec3 nn; 
			Ray ray = new_Ray();
			ray.o = rtask_ptr->ray.o;
			ray.d = rtask_ptr->ray.d;
			ray.tmax = rtask_ptr->ray.tmax;
			int next_triangle = intersect_scene(ray, tt, nn);
			if (next_triangle >= 0) {
				rtask_ptr->triangle_index = next_triangle;
				rtask_ptr->triangle_normal = nn;
				rtask_ptr->ray = ray;
			} else {
				rtask_ptr->finished = true;
			}
		}
	}
	
	//-------- trace 'em --------
	uniform uint first_ray_in_minibatch_offset = (pixels_per_mini_batch * minibatch) * G.num_offsets;
	uniform uint rays_left_in_minibatch = max_num_pixels * G.num_offsets;
	uniform uint last_ray_in_minibatch_offset = first_ray_in_minibatch_offset + rays_left_in_minibatch;

	// assign output buffer index to each rtask and clear it to black (reuse)
	foreach (ray_index_minibatch = first_ray_in_minibatch_offset ... last_ray_in_minibatch_offset)
	{
		RayTask* rtask = G.ray_tasks + ray_index_minibatch;
		rtask->buf_index = ray_index_minibatch;
		G.output_tmp[rtask->buf_index] = new_vec3();
	}

	// for each bounce..
	for (uniform uint i = 0; i < G.max_ray_depth; i++)
	{
		// trace all the rays for one bounce
		trace_rays(first_ray_in_minibatch_offset, rays_left_in_minibatch);

		// reorder them (scan) to pack the unfinished rays together
		gather_rays(first_ray_in_minibatch_offset, rays_left_in_minibatch, true, Unfinished);
		if (rays_left_in_minibatch == 0) {
			break;
		}

		// reorder them based on the material they hit
		// starts being beneficial when there're 3 materials (diffuse + mirror + glass)
		sort_rays_bsdf(first_ray_in_minibatch_offset, rays_left_in_minibatch);
	}
	
	//-------- average results and store into output buffer --------
	foreach (pixel_index_mini = 0 ... max_num_pixels)
	{
		uint pixel_index_local = pixel_index_mini + pixels_per_mini_batch * minibatch;
		uint pixel_index = pixel_index_local + batch * pixels_per_batch;
		if (pixel_index < buf_size)
		{
			vec3 v = new_vec3();
			for (int ray_index = 0; ray_index < G.num_offsets; ray_index++) {
				v = vadd(v, clamp01(G.output_tmp[ pixel_index_local * G.num_offsets + ray_index ]));
			}
			v = gamma_correct( smul(ray_weight, v) );
			set_color(pixel_index, v);
		}
	}
}
*/

void gather_test(uniform uint* uniform input, uniform uint* uniform output, uniform uint count)
{
	foreach (i = 0 ... 16) {
		output[i] = input[i];
	}
}

export void raytrace_scene_ispc(
	uniform Camera camera[], // pass in exactly 1 camera
	uniform float pixel_offsets[], // even: x; odd: y;
	uniform uint num_offsets,
	uniform Triangle triangles[],
	uniform BSDF bsdfs[],
	uniform uint area_light_indices[],
	uniform uint num_triangles,
	uniform uint num_area_lights,
	uniform uint8 output[],
	uniform uint width,
	uniform uint height,
	uniform bool rendering_to_tile,
	uniform uint tile_size,
	uniform uint tile_width,
	uniform uint tile_height,
	uniform uint tile_indexX,
	uniform uint tile_indexY,
	uniform uint num_threads,
	uniform uint max_ray_depth,
	uniform float rr_threshold,
	uniform bool use_direct_light,
	uniform uint area_light_samples,
	uniform BVH bvh_root[],
	uniform uint bvh_stack_size,
	uniform bool use_bvh,
	uniform bool use_dof,
	uniform float focal_distance,
	uniform float aperture_radius
){
	// create task queue and enqueue all the initial tasks (camera rays)
	// dequeue and execute?

	G.camera = camera;
	G.pixel_offsets = pixel_offsets;
	G.num_offsets = num_offsets;
	G.triangles = triangles;
	G.bsdfs = bsdfs;
	G.area_light_indices = area_light_indices;
	G.bvh_root = bvh_root;
	G.bvh_stack_size = bvh_stack_size;
	G.use_bvh = use_bvh;
	G.num_triangles = num_triangles;
	G.num_bsdfs = num_triangles;
	G.num_area_lights = num_area_lights;
	G.width = width;
	G.height = height;
	G.rendering_to_tile = rendering_to_tile;
	G.tile_size = tile_size;
	G.tile_width = tile_width;
	G.tile_height = tile_height;
	G.tile_indexX = tile_indexX;
	G.tile_indexY = tile_indexY;
	G.max_ray_depth = max_ray_depth;
	G.output = output;
	G.rr_threshold = rr_threshold;
	G.use_direct_light = use_direct_light;
	G.use_dof = use_dof;
	G.focal_distance = focal_distance;
	G.aperture_radius = aperture_radius;
	G.area_light_samples = area_light_samples;
	G.area_light_sample_weight = 1.0f / area_light_samples;

	G.output_tmp = uniform new uniform vec3[QUEUE_SIZE];

	// assign bsdf index
	foreach (i = 0 ... G.num_triangles) {
		G.triangles[i].bsdf_index = i;
	}

	// seed the RNGs
	foreach (i = 0 ... programCount) {
		seed_rng(&G.rng, i);
	}

	uniform uint buf_size = rendering_to_tile ? tile_width * tile_height : width * height;
	uniform uint buf_size3 = buf_size * 3;
	uniform uint buf_size3_full = rendering_to_tile ? tile_size*tile_size*3 : buf_size3;

	// initialize buffer
	foreach (i = 0 ... buf_size3_full) {
		output[i] = 128;
	}

	// initialize G.ray_tasks
	G.ray_tasks = uniform new uniform RayTask[QUEUE_SIZE];
	G.ray_tasks_backup = uniform new uniform RayTask[QUEUE_SIZE];
	foreach (i = 0 ... QUEUE_SIZE) {
		G.ray_tasks[i] = new_RayTask();
	}

	/////////////////////////////////////////////////

	uniform uint pixels_per_batch = QUEUE_SIZE / G.num_offsets;
	uniform uint num_batches = (buf_size + pixels_per_batch - 1) / pixels_per_batch;	

	uniform float ray_weight = 1.0f / G.num_offsets;
	uniform uint pixels_per_mini_batch = pixels_per_batch / num_threads;
	uniform uint num_mini_batches = (pixels_per_batch + pixels_per_mini_batch - 1) / pixels_per_mini_batch;

	for (uniform uint batch = 0; batch < num_batches; batch++)
	{
		// trace rays for the current batch
		//launch[num_mini_batches] raytrace_scene_ispc_task(batch, pixels_per_mini_batch, pixels_per_batch, buf_size, ray_weight);
		sync;
	}

	// TODO: optimize by reusing such small buffers (by allocating on cpp then pass in)
	delete[] G.output_tmp;
	delete[] G.ray_tasks;
	delete[] G.ray_tasks_backup;

	////////////////DEBUG////////////////

	#if 0
	uniform uint* uniform input_arr = uniform new uniform uint[16];
	uniform uint* uniform output_arr = uniform new uniform uint[16];

	foreach (i = 0 ... 16) {
		input_arr[i] = i;
	}

	gather_test(input_arr, output_arr, 16);

	foreach (i = 0 ... 16) {
		print("out: %", output_arr[i]);
	}

	delete[] input_arr;
	delete[] output_arr;
	#endif
}
