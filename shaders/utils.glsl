#ifndef _SHADER_INCLUDE_UTILS
#define _SHADER_INCLUDE_UTILS

precision highp float;

#define EPSILON 0.001f
#define PI 3.14159265359f
#define HALF_PI 1.57079632679f
#define ONE_OVER_PI 0.31830988618f
#define TWO_PI 6.28318530718f
#define ONE_OVER_TWO_PI 0.15915494309f

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

#endif