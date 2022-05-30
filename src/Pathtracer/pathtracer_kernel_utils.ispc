#define QUEUE_SIZE 2048
#define SCAN_SIZE 2048

struct vec3 {
	float x;
	float y;
	float z;
};

struct mat3 {
	vec3 colx;
	vec3 coly;
	vec3 colz;
};

struct Ray {
	vec3 o, d;
	float tmin, tmax;
	int receive_le;
};

struct RayTask {
	Ray ray;
	Ray ray_tmp; // buffer use only
	vec3 contribution;
	uint buf_index;

	vec3 triangle_normal;
	uint triangle_index;

	uint depth;
	int ray_tmp_hit_primitive; // buffer use only
	vec3 v0_buf; // buffer use only
	vec3 v1_buf; // buffer use only
	float s0_buf; // buffer use only
	int finished;
};

struct Camera {
	mat3 camera_to_world_rotation;
	vec3 position;
	float fov;
	float aspect_ratio;
};

struct Triangle {
	int bsdf_index;
	vec3 vertices[3];
	vec3 enormals[3];

	vec3 plane_n;
	float plane_k;
	float area;
};

struct BVH {
	vec3 min;
	vec3 max;
	uint triangles_start;
	uint triangles_count;
	int self_index;
	int left_index;
	int right_index;
};

#define NUM_MATERIAL_TYPES 4
enum BSDF_t {
	Diffuse,
	Mirror,
	Glass
};

struct BSDF {
	vec3 albedo;
	vec3 Le;
	BSDF_t type;
	bool is_delta;
	bool is_emissive;
};

struct Global
{
	varying RNGState rng;

	uniform Camera* camera;

	uniform float* pixel_offsets;
	uniform uint num_offsets;

	uniform Triangle* triangles;
	uniform BSDF* bsdfs;
	uniform uint* area_light_indices;
	uniform BVH* bvh_root;
	uniform uint bvh_stack_size;
	uniform bool use_bvh;

	uniform uint num_triangles;
	uniform uint num_bsdfs;
	uniform uint num_area_lights;

	uniform uint width, height;
	uniform bool rendering_to_tile;
	uniform uint tile_width, tile_height, tile_size, tile_indexX, tile_indexY;

	uniform uint max_ray_depth;
	uniform float rr_threshold;
	uniform bool use_direct_light;
	uniform uint area_light_samples;
	uniform float area_light_sample_weight;

	uniform bool use_dof;
	uniform float focal_distance;
	uniform float aperture_radius;

	uniform RayTask* uniform ray_tasks;
	uniform RayTask* uniform ray_tasks_backup;

	uniform vec3* uniform output_tmp; // a temporary buffer that stores float
	uniform uint8* uniform output;
};

uniform Global G;

enum RTaskBuffer {
	RTaskBufferA,
	RTaskBufferB
};

//-------- math --------

#define ONE_OVER_PI 0.31830988618f
#define EPSILON 0.001f

inline vec3 new_vec3() {
	vec3 v; v.x = 0.0f; v.y = 0.0f; v.z = 0.0f;
	return v;
}

inline vec3 new_vec3(float x, float y, float z) {
	vec3 v; v.x = x; v.y = y; v.z = z;
	return v;
}

inline mat3 new_mat3() {
	mat3 m;
	m.colx = new_vec3(1.0f, 0.0f, 0.0f);
	m.coly = new_vec3(0.0f, 1.0f, 0.0f);
	m.colz = new_vec3(0.0f, 0.0f, 1.0f);
	return m;
}

inline mat3 transpose(mat3 m0) {
	mat3 m = m0;
	m.colx.y = m0.coly.x;
	m.colx.z = m0.colz.x;
	m.coly.x = m0.colx.y;
	m.coly.z = m0.colz.y;
	m.colz.x = m0.colx.z;
	m.colz.y = m0.coly.z;
	return m;
}

inline vec3 neg(vec3 v) {
	vec3 res;
	res.x = -v.x;
	res.y = -v.y;
	res.z = -v.z;
	return res;
}

inline vec3 vadd(vec3 a, vec3 b) {
	vec3 res;
	res.x = a.x + b.x;
	res.y = a.y + b.y;
	res.z = a.z + b.z;
	return res;
}

inline vec3 vsub(vec3 a, vec3 b) {
	vec3 res;
	res.x = a.x - b.x;
	res.y = a.y - b.y;
	res.z = a.z - b.z;
	return res;
}

inline vec3 vmul(vec3 a, vec3 b) {
	vec3 res;
	res.x = a.x * b.x;
	res.y = a.y * b.y;
	res.z = a.z * b.z;
	return res;
}

inline vec3 smul(float a, vec3 v) {
	v.x *= a;
	v.y *= a;
	v.z *= a;
	return v;
}

inline vec3 mmul(mat3 m, vec3 v) {
	vec3 res;
	res.x = m.colx.x * v.x + m.coly.x * v.y + m.colz.x * v.z;
	res.y = m.colx.y * v.x + m.coly.y * v.y + m.colz.y * v.z;
	res.z = m.colx.z * v.x + m.coly.z * v.y + m.colz.z * v.z;
	return res;
}

inline float dot(vec3 a, vec3 b) {
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline vec3 cross(vec3 a, vec3 b) {
	vec3 res;
	res.x = a.y * b.z - b.y * a.z;
	res.y = -a.x * b.z + b.x * a.z;
	res.z = a.x * b.y - b.x * a.y;
	return res;
}

inline vec3 normalized(vec3 v) {
	float inv_len = 1.0f / sqrt(dot(v, v));
	return smul(inv_len, v);
}

inline vec3 clamp01(vec3 v) {
	vec3 res = v;
	if (res.x < 0) res.x = 0; if (res.x > 1) res.x = 1;
	if (res.y < 0) res.y = 0; if (res.y > 1) res.y = 1;
	if (res.z < 0) res.z = 0; if (res.z > 1) res.z = 1;
	return res;
}

inline float fmax(float a, float b) {
	if (a > b) return a;
	else return b;
}

inline float fmin(float a, float b) {
	if (a < b) return a;
	else return b;
}

inline float brightness(vec3 color) {
	return 0.2989f * color.x + 0.587f * color.y + 0.114f * color.z;
}

inline vec3 gamma_correct(vec3 v) {
	float gamma = 0.455f;
	vec3 res;
	res.x = pow(v.x, gamma);
	res.y = pow(v.y, gamma);
	res.z = pow(v.z, gamma);
	return res;
}

//-------- RNG, sampling --------

inline float rand01() {
	float r = abs(frandom(&G.rng));
	return r - floor(r);
}

inline float len(float x, float y) {
	return sqrt(x * x + y * y);
}

inline vec3 sample_hemisphere_cos_weighed() {
	float x = rand01() - 0.5f;
	float y = rand01() - 0.5f;
	while (len(x, y) > 0.5f) {
		x = rand01() - 0.5f;
		y = rand01() - 0.5f;
	}
	x *= 2.0f; y *= 2.0f;
	float l = len(x, y);
	float z = sqrt(1.0f - l * l);
	
	vec3 res = new_vec3(x, y, z);
	return res;
}

inline vec3 sample_unit_disc() {
	float x = rand01() - 0.5f;
	float y = rand01() - 0.5f;
	while (len(x, y) > 0.5f) {
		x = rand01() - 0.5f;
		y = rand01() - 0.5f;
	}
	x *= 2.0f; y *= 2.0f;
	vec3 res = new_vec3(x, y, 0);
	return res;
}

inline vec3 sample_point_in_triangle(Triangle* T)
{
	float u = rand01();
	float v = rand01();
	if (u + v > 1) {
		u = 1.0f - u;
		v = 1.0f - v;
	}

	vec3 e1 = vsub( T->vertices[1], T->vertices[0] );
	vec3 e2 = vsub( T->vertices[2], T->vertices[0] );

	return vadd( T->vertices[0], vadd(smul(u, e1), smul(v, e2)) );
}

//-------- scene, scene primitives --------

inline Ray new_Ray() {
	Ray r;
	r.o = new_vec3();
	r.d = new_vec3();
	r.tmin = 0.0;
	r.tmax = 99999999.0;
	r.receive_le = false;
	return r;
}

inline RayTask new_RayTask() {
	RayTask rt;
	rt.ray = new_Ray();
	rt.ray_tmp = new_Ray();
	rt.contribution = new_vec3(1.0f, 1.0f, 1.0f);
	rt.depth = 0;
	rt.buf_index = 0;
	rt.triangle_normal = new_vec3();
	rt.triangle_index = -1;
	rt.finished = true;
	return rt;
}

//-------- parallel data processing --------

inline void scan_excl(uniform uint array[], uniform uint length)
{
	uniform uint batches = (length + programCount - 1) / programCount;
	uniform uint offsets[programCount];
	foreach (i = 0 ... programCount) offsets[i] = 0;
	for (uniform uint i = 0; i < batches; i++)
	{
		foreach (j = 0 ... programCount)
		{
			int ind = i * programCount + j;
			int numOut = array[ind];
			int out = exclusive_scan_add(numOut) + offsets[j];
			array[ind] = out;
			offsets[j] = array[(i+1) * programCount - 1];
			if (extract(numOut, programCount-1) == 1) offsets[j] += 1;
		}
	}
}

inline void copy_rays_to_alternative_buffer(uniform uint tasks_offset, uniform uint num_tasks, bool to_backup)
{
	uniform RayTask* ray_tasks_in = to_backup ? G.ray_tasks : G.ray_tasks_backup;
	uniform RayTask* ray_tasks_out = to_backup ? G.ray_tasks_backup : G.ray_tasks;
	for (uniform uint i = tasks_offset; i < tasks_offset + num_tasks; i++)
	{
		*(ray_tasks_out + i) = *(ray_tasks_in + i);
	}
}

// since function pointer doesn't work, using this workaround..
enum RTaskCondition {
	Unfinished,
	NotDelta,
	S0Flag,
	Nothing
};

inline bool resolve_rtask_condition(RayTask* rtask, RTaskCondition condition)
{
	if (condition == Unfinished) {
		return !rtask->finished;
	}
	else if (condition == NotDelta) {
		Triangle* primitive = G.triangles + rtask->triangle_index;
		BSDF* bsdf = G.bsdfs + primitive->bsdf_index;
		return !bsdf->is_delta;	
	}
	else if (condition == S0Flag) {
		return rtask->s0_buf > 0.5f;
	}
	return true;
}

void gather_rays(
	uniform uint tasks_offset,
	uniform uint &num_tasks, // output: number of ray tasks that meet the condition and are packed to the beginning
	bool to_backup,
	RTaskCondition condition
) {
	uniform RayTask* ray_tasks_in = (to_backup ? G.ray_tasks : G.ray_tasks_backup) + tasks_offset;
	uniform RayTask* ray_tasks_out = (to_backup ? G.ray_tasks_backup : G.ray_tasks) + tasks_offset;

	uniform uint indices[SCAN_SIZE];

	uniform uint total = 0;
	uint partial = 0;

	foreach (i = 0 ... num_tasks) {
		bool cond = resolve_rtask_condition(ray_tasks_in + i, condition);
		indices[i] = (uint)cond;
		if (cond) partial++;
	}

	total = reduce_add(partial);
	scan_excl(indices, num_tasks);
	
	foreach (i = 0 ... num_tasks-1) {
		if (indices[i] != indices[i+1]) {
			ray_tasks_out[indices[i]] = *(ray_tasks_in + i);
		}
	}
	if (resolve_rtask_condition(ray_tasks_in+num_tasks-1, condition)) {
		ray_tasks_out[ indices[num_tasks-1] ] = *(ray_tasks_in + num_tasks - 1);
	}

	num_tasks = total;
}
