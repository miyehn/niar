
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
    bool receive_le;
};

struct RayTask {
    Ray ray;
    vec3 contribution;
    uint buf_index;

    vec3 triangle_normal;
    uint triangle_index;

    uint depth;
    bool finished;
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

    uniform uint num_triangles;
    uniform uint num_bsdfs;
    uniform uint num_area_lights;

    uniform uint width, height, max_ray_depth;
    uniform float rr_threshold;
    uniform bool use_direct_light;
    uniform uint area_light_samples;
    uniform float area_light_sample_weight;

    uniform RayTask* ray_tasks;
    uniform RayTask* ray_tasks_backup;

    uniform vec3* output_tmp; // a temporary buffer that stores float
    uniform uint8* output;
    //uniform bool finished[QUEUE_SIZE];
};

uniform Global G;

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

inline mat3 transpose(mat3& m0) {
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

inline vec3 mmul(mat3 &m, vec3 &v) {
    vec3 res;
    res.x = m.colx.x * v.x + m.coly.x * v.y + m.colz.x * v.z;
    res.y = m.colx.y * v.x + m.coly.y * v.y + m.colz.y * v.z;
    res.z = m.colx.z * v.x + m.coly.z * v.y + m.colz.z * v.z;
    return res;
}

inline float dot(vec3 &a, vec3 &b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline vec3 cross(vec3 &a, vec3 &b) {
    vec3 res;
    res.x = a.y * b.z - b.y * a.z;
    res.y = -a.x * b.z + b.x * a.z;
    res.z = a.x * b.y - b.x * a.y;
    return res;
}

inline vec3 normalized(vec3 &v) {
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
    rt.contribution = new_vec3(1.0f, 1.0f, 1.0f);
    rt.depth = 0;
    rt.buf_index = 0;
    rt.triangle_normal = new_vec3();
    rt.triangle_index = -1;
    rt.finished = true;
    return rt;
}

inline RayTask cpy_RayTask(RayTask* rt0) {
    RayTask rt;
    rt.ray.o = rt0->ray.o;
    rt.ray.d = rt0->ray.d;
    rt.ray.tmin = rt0->ray.tmin;
    rt.ray.tmax = rt0->ray.tmax;
    rt.ray.receive_le = rt0->ray.receive_le;
    rt.contribution = rt0->contribution;
    rt.depth = rt0->depth;
    rt.buf_index = rt0->buf_index;
    rt.triangle_normal = rt0->triangle_normal;
    rt.triangle_index = rt0->triangle_index;
    rt.finished = rt0->finished;
    return rt;
}

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

inline int intersect_bvh_triangles(int bvh_index, Ray& ray, float& t, vec3& normal, bool modify_ray, bool nearest)
{
    int triangle_index = -1;
    if (intersect_aabb(bvh_index, ray))
    {
        BVH* bvh = G.bvh_root + bvh_index;
        int l = bvh->left_index, r = bvh->right_index;
        if (l>=0 && r>=0)
        {
            triangle_index = intersect_bvh_triangles(l, ray, t, normal, modify_ray, nearest);
            int tmp = intersect_bvh_triangles(r, ray, t, normal, modify_ray, nearest);
            if (tmp >= 0) triangle_index = tmp;
        }
        else
        {
            for (uint i=bvh->triangles_start; i < bvh->triangles_start + bvh->triangles_count; i++) {
                Triangle* T = G.triangles + i;
                if (intersect(*T, ray, t, normal, modify_ray)) {
                    triangle_index = i;
                    if (!nearest) return triangle_index;
                }
            }
        }
    }
    return triangle_index;
}

inline int intersect_scene(Ray& ray, float& t, vec3& normal, bool modify_ray, bool nearest)
{
#if 1
    return intersect_bvh_triangles(0, ray, t, normal, modify_ray, nearest);
#else
    int triangle_index = -1;
    for (uint i = 0; i < G.num_triangles; i++) {
        if (intersect(G.triangles[i], ray, t, normal, modify_ray)) {
            triangle_index = i;
            if (!nearest) return triangle_index;
        }
    }
    return triangle_index;
#endif
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

inline float brightness(vec3& color) {
    return 0.2989f * color.x + 0.587f * color.y + 0.114f * color.z;
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
    int w = pixel_index % G.width; // TODO
    int h = pixel_index / G.width; // TODO
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

    // pre-compute (or declare) some common thing sto be used later
    vec3 L = new_vec3();
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
            L = vadd(L, bsdf->Le);
        }
    } else {
        L = vadd(L, bsdf->Le);
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
                #if 1
                bool in_shadow = intersect_scene(ray_to_light, tmp_t, tmp_n, false, false) >= 0;
                #else
                bool in_shadow = false;
                Triangle* hit_prim;
                for (uint k = 0; k < G.num_triangles; k++) {
                    if (intersect(G.triangles[k], ray_to_light, tmp_t, tmp_n, false)) {
                        hit_prim = G.triangles + k;
                        if (hit_prim != primitive && hit_prim != area_light_T) {
                            in_shadow = true;
                            break;
                        }
                    }
                }
                #endif

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
                    L = vadd( L, smul(G.area_light_sample_weight, L_direct) );
                }
            }
        }
    }

    vec3* output_ptr = G.output_tmp + rtask.buf_index;
    *output_ptr = vadd(*output_ptr, vmul(rtask.contribution, L));

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
            int next_triangle = intersect_scene(ray, tt, rtask.triangle_normal, true, true);
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
    rtask.depth += 1;
    
}

inline void scan_excl(uniform uint array[], uniform uint length)
{
    if (length < 1) return;

    // upsweep phase
    for (uniform uint twod = 1; twod < length; twod *= 2) {
        uniform uint twod1 = twod*2;
        uniform uint limit = length / twod1;
        foreach (i = 0 ... limit) {
            array[i*twod1+twod1-1] += array[i*twod1+twod-1];
        }
    }
    array[length - 1] = 0;

    for (uniform uint twod = length / 2; twod >= 1; twod /= 2)
    {
        uniform uint twod1 = twod * 2;
        uniform uint limit = length / twod1;
        foreach (i = 0 ... limit)
        {
            uint t = array[i*twod1+twod-1];
            array[i*twod1+twod-1] = array[i*twod1+twod1-1];
            array[i*twod1+twod1-1] += t;
        }
    }

}

inline uniform uint nextPow2(uniform uint n) {
    n--;
    n = n | n >> 1;
    n = n | n >> 2;
    n = n | n >> 4;
    n = n | n >> 8;
    n = n | n >> 16;
    n++;
    return n;
}

inline void gather_rays(uniform uint tasks_offset, uniform uint &num_tasks)
{
    uniform RayTask* ray_tasks_in = G.ray_tasks + tasks_offset;
    uniform RayTask* ray_tasks_out = G.ray_tasks_backup + tasks_offset;

    uniform uint num_tasks_pad = nextPow2(num_tasks);
    uniform uint indices[SCAN_SIZE];
    // foreach (i = 0 ... SCAN_SIZE) indices[i] = 0;

    uniform uint total = 0;
    uint partial = 0;

    foreach (i = 0 ... num_tasks) {
        indices[i] = (uint)(!ray_tasks_in[i].finished);
        if (!ray_tasks_in[i].finished) partial++;
    }

    total = reduce_add(partial);
    scan_excl(indices, num_tasks_pad);
    
    foreach (i = 0 ... num_tasks-1) {
        if (indices[i] != indices[i+1]) {
            ray_tasks_out[indices[i]] = cpy_RayTask(ray_tasks_in + i);
        }
    }
    if (!ray_tasks_in[num_tasks-1].finished) {
        ray_tasks_out[ indices[num_tasks-1] ] = cpy_RayTask(ray_tasks_in + num_tasks - 1);
    }

    num_tasks = total;
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

    uniform uint num_tasks_pad = nextPow2(num_tasks);

    uniform uint indices[SCAN_SIZE];
    uniform uint old_idx = 0;
    for (uniform uint cur_bsdf = 0; cur_bsdf < NUM_MATERIAL_TYPES; cur_bsdf++) {

        //foreach (i = 0 ... SCAN_SIZE) indices[i] = 0;
        foreach (i = 0 ... num_tasks) {
            uint bsdf_type = (uint)get_bsdf_t(ray_tasks_in, i);
            indices[i] = (bsdf_type == cur_bsdf);
        }
        scan_excl(indices, num_tasks_pad);
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
                ray_tasks_out_local[indices[i]] = cpy_RayTask(ray_tasks_in + i);
            }
        }
        if (last_bsdf == cur_bsdf) {
            ray_tasks_out_local[ indices[num_tasks-1] ] = cpy_RayTask(ray_tasks_in + num_tasks - 1);
        }
        old_idx = old_idx+idx;
    }
}

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
            int next_triangle = intersect_scene(ray, tt, nn, true, true);
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

    foreach (ray_index_minibatch = first_ray_in_minibatch_offset ... last_ray_in_minibatch_offset)
    {
        RayTask* rtask = G.ray_tasks + ray_index_minibatch;
        rtask->buf_index = ray_index_minibatch;
        G.output_tmp[rtask->buf_index] = new_vec3();
    }

    for (uniform uint i = 0; i < G.max_ray_depth / 2; i++)
    {
        // trace all the rays for one bounce
        foreach (ray_index_minibatch = first_ray_in_minibatch_offset ... first_ray_in_minibatch_offset + rays_left_in_minibatch)
        {
            RayTask* rtask_ptr = G.ray_tasks + ray_index_minibatch;
            RayTask rtask = cpy_RayTask(rtask_ptr);
            trace_ray(rtask);
            *rtask_ptr = rtask;
        }

        // reorder them (scan)
        gather_rays(first_ray_in_minibatch_offset, rays_left_in_minibatch);
        if (rays_left_in_minibatch == 0) {
            break;
        }

        // reorder them based on the material they hit
        // currently this actually increases runtime by ~5%, will see how it goes when more materials are added.
        // (note: I have no idea why merging the below two function calls result in weird memory accesses)
#if 1
        //uniform uint bsdf_buckets[NUM_MATERIAL_TYPES];
        sort_rays_bsdf(first_ray_in_minibatch_offset, rays_left_in_minibatch);
#else
        foreach (j = 0 ... rays_left_in_minibatch) {
            *(G.ray_tasks + first_ray_in_minibatch_offset + j) = 
                cpy_RayTask(G.ray_tasks_backup + first_ray_in_minibatch_offset + j);
        }
#endif
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
            v = smul(ray_weight, v);
            set_color(pixel_index, v);
        }
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
    uniform uint num_threads,
    uniform uint max_ray_depth,
    uniform float rr_threshold,
    uniform bool use_direct_light,
    uniform uint area_light_samples,
    uniform BVH bvh_root[]
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
    G.num_triangles = num_triangles;
    G.num_bsdfs = num_triangles;
    G.num_area_lights = num_area_lights;
    G.width = width;
    G.height = height;
    G.max_ray_depth = max_ray_depth;
    G.output = output;
    G.rr_threshold = rr_threshold;
    G.use_direct_light = use_direct_light;
    G.area_light_samples = area_light_samples;
    G.area_light_sample_weight = 1.0f / area_light_samples;

    G.output_tmp = uniform new vec3[QUEUE_SIZE];

    // assign bsdf index
    foreach (i = 0 ... G.num_triangles) {
        G.triangles[i].bsdf_index = i;
    }

    // seed the RNGs
    foreach (i = 0 ... programCount) {
        seed_rng(&G.rng, i);
    }

    uniform uint buf_size = width * height;
    uniform uint buf_size3 = buf_size * 3;

    // initialize buffer
    foreach (i = 0 ... buf_size3) {
        output[i] = 128;
    }

    // initialize G.ray_tasks
    G.ray_tasks = uniform new RayTask[QUEUE_SIZE];
    G.ray_tasks_backup = uniform new RayTask[QUEUE_SIZE];
    foreach (i = 0 ... QUEUE_SIZE) {
        G.ray_tasks[i] = new_RayTask();
    }

    /*
    BVH* bvh = G.bvh_root;
    vec3 min = bvh->min;
    vec3 max = bvh->max;
    print("(% % %), (% % %)\n", min.x, min.y, min.z, max.x, max.y, max.z);
    
    for (uniform uint i=0; i<7; i++) {
        uniform BVH* bvh = G.bvh_root + i;
        print("%-(%,%) : % -> %\n", bvh->self_index, bvh->left_index, bvh->right_index, bvh->triangles_start, bvh->triangles_start + bvh->triangles_count);
    }
    */

    /////////////////////////////////////////////////

    uniform uint pixels_per_batch = QUEUE_SIZE / G.num_offsets;
    uniform uint num_batches = (buf_size + pixels_per_batch - 1) / pixels_per_batch;    

    uniform float ray_weight = 1.0f / G.num_offsets;
    uniform uint pixels_per_mini_batch = pixels_per_batch / num_threads;
    uniform uint num_mini_batches = (pixels_per_batch + pixels_per_mini_batch - 1) / pixels_per_mini_batch;

    for (uniform uint batch = 0; batch < num_batches; batch++)
    {
        // trace rays for the current batch
        launch[num_mini_batches] raytrace_scene_ispc_task(batch, pixels_per_mini_batch, pixels_per_batch, buf_size, ray_weight);
        sync;
    }

    delete[] G.output_tmp;
    delete[] G.ray_tasks;
    delete[] G.ray_tasks_backup;
}
