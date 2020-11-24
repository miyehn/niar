
#define QUEUE_SIZE 1024
#define MINI_BATCH_GRAN 1

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
    float rr_contribution;
    bool receive_le;
};

struct RayTask {
    Ray ray;
    vec3 contribution;
    uint depth;
    uint buf_index;
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
};

// diffuse only..
struct BSDF {
    bool is_delta;
    bool is_emissive;
    vec3 albedo;
    vec3 Le;
};

struct Global
{
    varying RNGState rng;

    uniform Camera* camera;

    uniform float* pixel_offsets;
    uniform uint num_offsets;

    uniform Triangle* triangles;
    uniform BSDF* bsdfs;

    uniform uint num_triangles;
    uniform uint num_bsdfs;

    uniform uint width, height, max_ray_depth;

    uniform RayTask* ray_tasks;
    uniform RayTask* ray_tasks_backup;
    uniform uint scratch[QUEUE_SIZE];

    uniform vec3* output_tmp; // a temporary buffer that stores float
    uniform uint8* output;
};

uniform Global G;

//-------- math --------

#define ONE_OVER_PI 0.31830988618f
#define EPSILON 0.001f

#define RR_THRESHOLD 0.05f

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

inline vec3 clamp01(vec3 &v) {
    vec3 res = v;
    if (res.x < 0) res.x = 0; if (res.x > 1) res.x = 1;
    if (res.y < 0) res.y = 0; if (res.y > 1) res.y = 1;
    if (res.z < 0) res.z = 0; if (res.z > 1) res.z = 1;
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

//-------- scene, scene primitives, BSDF --------

inline Ray new_Ray() {
    Ray r;
    r.o = new_vec3();
    r.d = new_vec3();
    r.tmin = 0.0;
    r.tmax = 99999999.0;
    r.rr_contribution = 1.0f;
    r.receive_le = false;
    return r;
}

inline RayTask new_RayTask() {
    RayTask rt;
    rt.ray = new_Ray();
    rt.contribution = new_vec3(1.0f, 1.0f, 1.0f);
    rt.depth = 0;
    rt.buf_index = 0;
    rt.finished = true;
    return rt;
}

inline RayTask cpy_RayTask(RayTask* rt0) {
    RayTask rt;
    rt.ray.o = rt0->ray.o;
    rt.ray.d = rt0->ray.d;
    rt.ray.tmin = rt0->ray.tmin;
    rt.ray.tmax = rt0->ray.tmax;
    rt.ray.rr_contribution = rt0->ray.rr_contribution;
    rt.ray.receive_le = rt0->ray.receive_le;
    rt.contribution = rt0->contribution;
    rt.depth = rt0->depth;
    rt.buf_index = rt0->buf_index;
    rt.finished = rt0->finished;
    return rt;
}

inline vec3 BSDF_f(BSDF* bsdf, vec3& wi, vec3& wo) {
    return smul(ONE_OVER_PI, bsdf->albedo);
}

inline vec3 BSDF_sample_f(BSDF* bsdf, float& pdf, vec3& wi, vec3 wo) {
    wi = sample_hemisphere_cos_weighed();
    pdf = wi.z * ONE_OVER_PI;
    return BSDF_f(bsdf, wi, wo);
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
    if (rtask.finished) return;

    Ray &ray = rtask.ray;

    // info of closest hit
    Triangle* primitive = NULL;
    BSDF* bsdf = NULL;
    float t; vec3 n;
    for (uint i = 0; i < G.num_triangles; i++) {
        if (intersect(G.triangles[i], ray, t, n, true)) {
            primitive = G.triangles + i;
            bsdf = G.bsdfs + primitive->bsdf_index;
        }
    }
    if (primitive)
    {
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

        L = vadd(L, bsdf->Le);
        vec3* output_ptr = G.output_tmp + rtask.buf_index;
        *output_ptr = vadd(*output_ptr, vmul(rtask.contribution, L));

        //---- indirect lighting ----

        float pdf;
        vec3 f = BSDF_sample_f(bsdf, pdf, wi_hemi, wo_hemi);

        // transform wi back to world space
        wi_world = mmul(h2w, wi_hemi);
        costhetai = abs(dot(n, wi_world));

        // russian roulette
        float termination_prob = 0.0f;
        ray.rr_contribution *= brightness(f) * costhetai;
        if (ray.rr_contribution < RR_THRESHOLD) {
            termination_prob = (RR_THRESHOLD - ray.rr_contribution) / RR_THRESHOLD;
        }
        bool terminate = rand01() < termination_prob;

        // recursive step: trace scattered ray in wi direction (if not terminated by RR)
        vec3 Li = new_vec3();
        if (!terminate) {
            vec3 refl_offset = wi_hemi.z > 0 ? smul(EPSILON, n) : neg( smul(EPSILON, n) );
            Ray ray_refl = new_Ray();
            ray_refl.o = vadd(hit_p, refl_offset);
            ray_refl.d = wi_world;
            rtask.ray = ray_refl;
            rtask.contribution = vmul(rtask.contribution,
                smul(costhetai / pdf * (1.0f / (1.0f - termination_prob)), f));
        } else {
            rtask.finished = true;
        }

        rtask.depth += 1;
        if (rtask.depth >= G.max_ray_depth) {
            rtask.finished = true;
        }
    }
    else
    {
        rtask.finished = true;
    }
}

// TODO: make parallel
inline void scan_excl(uniform uint* array, uniform uint length)
{
    if (length < 1) return;
    // scan inclusive
    for (uniform uint i = 1; i < length; i++) {
        array[i] += array[i-1];
    }
    // make exclusive by shifting values by 1
    for (uniform uint i = length-1; i >= 1; i--) {
        array[i] = array[i-1];
    }
    array[0] = 0;
}

inline void gather_rays(uniform uint tasks_offset, uniform uint &num_tasks)
{
    uniform RayTask* ray_tasks = G.ray_tasks + tasks_offset;
    uniform RayTask* ray_tasks_backup = G.ray_tasks_backup + tasks_offset;
    uniform uint* indices = G.scratch + tasks_offset;
    
    uniform uint total = 0;
    uint partial = 0;

    foreach (i = 0 ... num_tasks) {
        indices[i] = (uint)(!ray_tasks[i].finished);
        if (!ray_tasks[i].finished) partial++;
    }
    total = reduce_add(partial);

    scan_excl(indices, num_tasks);

    foreach (i = 0 ... num_tasks-1) {
        if (indices[i] != indices[i+1]) {
            ray_tasks_backup[indices[i]] = cpy_RayTask(ray_tasks + i);
        }
    }
    if (!ray_tasks[num_tasks-1].finished) {
        ray_tasks_backup[ indices[num_tasks-1] + 1 ] = cpy_RayTask(ray_tasks + num_tasks - 1);
    }

    num_tasks = total;
    foreach (i = 0 ... num_tasks) {
        ray_tasks[i] = cpy_RayTask(ray_tasks_backup + i);
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

    for (uniform uint i = 0; i < G.max_ray_depth; i++)
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
        // gives a ~10% speedup...should perform better when scan is made parallel
        gather_rays(first_ray_in_minibatch_offset, rays_left_in_minibatch);
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
                v = vadd(v, G.output_tmp[ pixel_index_local * G.num_offsets + ray_index ]);
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
    uniform int num_triangles,
    uniform uint8 output[],
    uniform uint width,
    uniform uint height,
    uniform uint max_ray_depth
){
    // create task queue and enqueue all the initial tasks (camera rays)
    // dequeue and execute?

    G.camera = camera;
    G.pixel_offsets = pixel_offsets;
    G.num_offsets = num_offsets;
    G.triangles = triangles;
    G.bsdfs = bsdfs;
    G.num_triangles = num_triangles;
    G.num_bsdfs = num_triangles;
    G.width = width;
    G.height = height;
    G.max_ray_depth = max_ray_depth;
    G.output = output;


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

    /////////////////////////////////////////////////

    uniform uint pixels_per_batch = QUEUE_SIZE / G.num_offsets;
    uniform uint num_batches = (buf_size + pixels_per_batch - 1) / pixels_per_batch;    

    uniform float ray_weight = 1.0f / G.num_offsets;
    uniform uint pixels_per_mini_batch = pixels_per_batch / MINI_BATCH_GRAN;
    uniform uint num_mini_batches = (pixels_per_batch + pixels_per_mini_batch - 1) / pixels_per_mini_batch;

    for (uniform uint batch = 0; batch < num_batches; batch++)
    {
        // trace rays for the current batch
        launch[num_mini_batches] raytrace_scene_ispc_task(batch, pixels_per_mini_batch, pixels_per_batch, buf_size, ray_weight);
        sync;
    }

    /*
    uniform uint* test = uniform new uint[16];
    foreach (i = 0 ... 16) {
        if (i % 2 == 0) test[i] = 0;
        else test[i] = 1;
        print("% ", test[i]);
    }
    print("\n");
    scan_excl(test, 16);
    foreach (i = 0 ... 16) {
        print("% ", test[i]);
    }
    */

    delete[] G.output_tmp;
    delete[] G.ray_tasks;
    delete[] G.ray_tasks_backup;
}
