#ifndef _SHADER_INCLUDE_UTILS
#define _SHADER_INCLUDE_UTILS

precision highp float;

#define EPSILON 0.001f
#define PI 3.14159265359f
#define HALF_PI 1.57079632679f
#define ONE_OVER_PI 0.31830988618f
#define TWO_PI 6.28318530718f
#define ONE_OVER_TWO_PI 0.15915494309f

uint pcg_hash(uint value)
{
    uint state = value * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint hash_combine(uint a, uint b)
{
    return pcg_hash(a ^ pcg_hash(b + 0x9E3779B9u));
}

// Naming of white noise values: "whiteNoise" followed by what the value is unique to (every component of a vector
// value is unique to all of these, and independent of the others):
//   XY: the texel (pixel)
//   F:  the frame (WhiteNoiseF, which the CPU makes anew for every frame, is what makes noise unique per frame)
//   R:  the ray
//   B:  the bounce (a vertex of a path; shared by the bounce ray and the shadow rays shot from that vertex)
// e.g. whiteNoiseXYF is unique per texel per frame, but shared by all the rays shot from that texel in that frame.
// Parameters that need a particular kind of noise say so in their names. Neither the salt of white_noise01() nor the
// sample of sampleCosineWeightedHemisphere() carries such a suffix: they take whatever kind the caller has.
float white_noise01(uvec4 xyzw)
{
    uint h = hash_combine(xyzw.x, xyzw.w);
    h = hash_combine(h, xyzw.y);
    h = hash_combine(h, xyzw.z);
    return float(h >> 8u) * (1.0f / 16777216.0f);
}

// the result is unique to xyz and to the salt, which is any noise value the result should also be unique to
float white_noise01(uvec3 xyz, float salt)
{
    return white_noise01(uvec4(xyz, floatBitsToUint(salt)));
}

float white_noise01(uvec2 xy, float salt)
{
    return white_noise01(uvec3(xy, 0u), salt);
}

vec3 sampleLongLatMap(sampler2D map, vec3 dir, float mipLevel)
{
    float phi = atan(dir.y, dir.x);
    float theta = asin(dir.z);
    vec2 uv = vec2(-phi * ONE_OVER_TWO_PI + 0.5f, -theta * ONE_OVER_PI + 0.5f);
    return textureLod(map, uv, mipLevel).rgb;
}

vec3 uvToViewDir_ws(mat3 viewMatrixRot, float halfVerticalFov, float aspectRatio, vec2 uv) {
    vec2 ndc = vec2(uv.x * 2 - 1, (1.0-uv.y) * 2 - 1);
    float tanHalfFov = tan(halfVerticalFov);
    float ky = ndc.y * tanHalfFov;
    float kx = ndc.x * tanHalfFov * aspectRatio;
    vec3 dir = normalize(vec3(kx, ky, -1));
    mat3 c2w = transpose(viewMatrixRot);
    return c2w * dir;
}

vec3 screenSpaceUvToViewDir(vec2 uv, mat4 ViewMatrix, float halfVFovRadians, float aspectRatio)
{
    float yHalf = tan(halfVFovRadians);
    float xHalf = yHalf * aspectRatio;

    vec2 ndc = uv;
    ndc = ndc * 2.0f - 1.0f; // normalize to [-1, 1] on both axes
    ndc.y = -ndc.y;

    vec3 camSpaceDir = normalize(vec3(xHalf * ndc.x, yHalf * ndc.y, -1));
    mat3 camToWorldRot = transpose(mat3(ViewMatrix));
    return camToWorldRot * camSpaceDir;
}

vec3 reflectRay(vec3 normal, vec3 rayDir)
{
    vec3 n = dot(normal, rayDir) < 0.0f ? normal : -normal;
    return rayDir - 2.0f * dot(rayDir, n) * n;
}

vec3 buildTangent(vec3 normal)
{
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
    return normalize(cross(up, normal));
}

// u: a uniformly distributed random point in [0, 1)^2
vec3 sampleCosineWeightedHemisphere(vec3 normal, vec2 u)
{
    float r = sqrt(u.x);
    float phi = TWO_PI * u.y;
    vec3 localDir = vec3(r * cos(phi), r * sin(phi), sqrt(max(0.0, 1.0 - u.x)));

    vec3 tangent = buildTangent(normal);
    vec3 bitangent = cross(normal, tangent);
    return normalize(localDir.x * tangent + localDir.y * bitangent + localDir.z * normal);
}

#endif