export void test_kernel(uniform int input[], uniform int output[], uniform int len) {
    foreach (i = 0 ... len) {
        output[i] = input[i];
    }
}

//-------- math --------

struct vec3 {
    float x;
    float y;
    float z;
};

vec3 new_vec3() {
    vec3 v; v.x = 0.0f; v.y = 0.0f; v.z = 0.0f;
    return v;
}

struct mat3 {
    vec3 colx;
    vec3 coly;
    vec3 colz;
};

mat3 new_mat3() {
    mat3 m;
    m.colx = new_vec3();
    m.coly = new_vec3();
    m.colz = new_vec3();
    m.colx.x = 1.0f;
    m.coly.y = 1.0f;
    m.colz.z = 1.0f;
    return m;
}

vec3 vadd(vec3 a, vec3 b) {
    vec3 res;
    res.x = a.x + b.x;
    res.y = a.y + b.y;
    res.z = a.z + b.z;
    return res;
}

vec3 vsub(vec3 a, vec3 b) {
    vec3 res;
    res.x = a.x - b.x;
    res.y = a.y - b.y;
    res.z = a.z - b.z;
    return res;
}

vec3 vmul(vec3 a, vec3 b) {
    vec3 res;
    res.x = a.x * b.x;
    res.y = a.y * b.y;
    res.z = a.z * b.z;
    return res;
}

vec3 smul(float a, vec3 v) {
    v.x *= a;
    v.y *= a;
    v.z *= a;
    return v;
}

vec3 mmul(mat3 m, vec3 v) {
    vec3 res;
    res.x = m.colx.x * v.x + m.coly.x * v.y + m.colz.x * v.z;
    res.y = m.colx.y * v.x + m.coly.y * v.y + m.colz.y * v.z;
    res.z = m.colx.z * v.x + m.coly.z * v.y + m.colz.z * v.z;
    return res;
}

float dot(vec3 a, vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

vec3 normalized(vec3 v) {
    float inv_len = 1.0f / sqrt(dot(v, v));
    return smul(inv_len, v);
}

//-------- scene --------

struct Ray {
    vec3 o, d;
    double tmin, tmax;
    float rr_contribution;
    bool receive_le;
};

Ray new_Ray() {
    Ray r;
    r.o = new_vec3();
    r.d = new_vec3();
    r.tmin = 0.0;
    r.tmax = 99999999.0;
    r.rr_contribution = 1.0f;
    r.receive_le = false;
    return r;
}

struct RayTask {
    Ray ray;
    vec3 output; 
    vec3 contribution;
};

RayTask new_RayTask() {
    RayTask rt;
    rt.ray = new_Ray();
    rt.output = new_vec3();
    rt.contribution = new_vec3();
    return rt;
}

struct Triangle {
    vec3 vertices[3];
    vec3 normals[3];
    vec3 enormals[3];

    vec3 e1, e2;
    vec3 plane_n;
    float plane_k;
    float area;
};

bool intersect(Triangle& triangle, Ray& ray, double& t, vec3& normal, bool modify_ray) {
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

export void raytrace_scene_ispc(
    uniform Triangle triangles[],
    uniform int num_triangles,
    uniform vec3 output[],
    uniform int width,
    uniform int height
){
    // pass in the whole scene and output buffer
    // create task queue and enqueue all the initial tasks (camera rays)
    // dequeue and execute?

    Triangle wat = triangles[1];
}