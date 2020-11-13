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

struct mat3 {
    vec3 colx;
    vec3 coly;
    vec3 colz;
};

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

struct Triangle {

};

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