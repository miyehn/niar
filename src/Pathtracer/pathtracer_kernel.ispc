
#define QUEUE_SIZE 1024

struct Camera;
struct Triangle;
struct BSDF;

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
    double tmin, tmax;
    float rr_contribution;
    bool receive_le;
};

struct RayTask {
    Ray ray;
    vec3 output; 
    vec3 contribution;
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

    uniform uint width, height;

    uniform RayTask tasks_A[QUEUE_SIZE];
    uniform RayTask tasks_B[QUEUE_SIZE];
    uniform bool read_from_A;

    uniform uint8* output;
};

uniform Global G;

//-------- math --------

#define ONE_OVER_PI 0.31830988618f

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

inline vec3 vadd(vec3 &a, vec3 &b) {
    vec3 res;
    res.x = a.x + b.x;
    res.y = a.y + b.y;
    res.z = a.z + b.z;
    return res;
}

inline vec3 vsub(vec3 &a, vec3 &b) {
    vec3 res;
    res.x = a.x - b.x;
    res.y = a.y - b.y;
    res.z = a.z - b.z;
    return res;
}

inline vec3 vmul(vec3 &a, vec3 &b) {
    vec3 res;
    res.x = a.x * b.x;
    res.y = a.y * b.y;
    res.z = a.z * b.z;
    return res;
}

inline vec3 smul(float a, vec3 &v) {
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
    res.y = a.x * b.z - b.x * a.z;
    res.z = a.x * b.y - b.x * a.y;
    return res;
}

inline vec3 normalized(vec3 &v) {
    float inv_len = 1.0f / sqrt(dot(v, v));
    return smul(inv_len, v);
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

struct Camera {
    mat3 camera_to_world_rotation;
    vec3 position;
    float fov;
    float aspect_ratio;
};

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
    rt.output = new_vec3();
    rt.contribution = new_vec3();
    return rt;
}

// diffuse only..
struct BSDF {
    bool is_delta;
    bool is_emissive;
    vec3 albedo;
    vec3 Le;
};

inline vec3 BSDF_f(BSDF& bsdf, vec3& wi, vec3& wo) {
    return smul(ONE_OVER_PI, bsdf.albedo);
}

inline vec3 BSDF_sample_f(BSDF& bsdf, float& pdf, vec3& wi, vec3 wo) {
    wi = sample_hemisphere_cos_weighed();
    pdf = wi.z * ONE_OVER_PI;
    return BSDF_f(bsdf, wi, wo);
}

struct Triangle {
		int bsdf_index;
    vec3 vertices[3];
    vec3 enormals[3];

    vec3 plane_n;
    float plane_k;
};

inline bool intersect(Triangle& triangle, Ray& ray, double& t, vec3& normal, bool modify_ray) {
    // ray parallel to plane
    float d_dot_n = dot(ray.d, triangle.plane_n);
    if (abs(d_dot_n) == 0.0f) return false;
    // intesection out of range
    double _t = (triangle.plane_k - dot(ray.o, triangle.plane_n)) / d_dot_n;
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
    return 0.2989f * color.x + 0.587f * color.y + 0.114 * color.z;
}

inline void set_color(uint i, vec3 &color) {
    uint i3 = i * 3;
    G.output[i3] = 0xFF & (int)(color.x * 255.0f);
    G.output[i3 + 1] = 0xFF & (int)(color.y * 255.0f);
    G.output[i3 + 2] = 0xFF & (int)(color.z * 255.0f);
}

export void raytrace_scene_ispc(
    uniform Camera camera[], // pass in exactly 1 camera
    uniform float pixel_offsets[], // even: x; odd: y;
    uniform uint num_offsets,
    uniform Triangle triangles[],
    uniform BSDF bsdfs[],
    uniform int num_triangles,
    uniform uint8 output[],
    uniform int width,
    uniform int height
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
    G.output = output;
    G.read_from_A = true;

    // seed the RNGs
    foreach (i = 0 ... programCount) {
        seed_rng(&G.rng, i);
    }

    uniform uint buf_size = width * height;
    uniform uint buf_size3 = buf_size * 3;

    // initialize buffer
    foreach (i = 0 ... buf_size3) {
        output[i] = 127;
    }

    /////////////////////////////////////////////////

    uniform uint pixels_per_batch = QUEUE_SIZE / G.num_offsets;
    uniform uint num_batches = (buf_size + pixels_per_batch - 1) / pixels_per_batch;

    for (uniform uint batch = 0; batch < num_batches; batch++)
    {
        foreach (pixel_index_local = 0 ... pixels_per_batch, ray_index = 0 ... G.num_offsets)
        {
            uint pixel_index = pixel_index_local + batch * pixels_per_batch;

            // TODO: make a superbuffer (float*) in G for each ray to write to, 
            // and reuse that for each batch.

            //uint queue_slot = pixel_index + ray_index;

            /*
            vec3 v;
            vec3 half = new_vec3(0.5f, 0.5f, 0.0f);
            v = vadd(half, sample_hemisphere_cos_weighed());
            set_color(pixel_index, v);
            */
        }
    }
}
