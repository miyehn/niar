#define EPSILON 0.001f
#define PI 3.14159265359f
#define HALF_PI 1.57079632679f
#define ONE_OVER_PI 0.31830988618f
#define TWO_PI 6.28318530718f
#define ONE_OVER_TWO_PI 0.15915494309f

vec3 sampleLongLatMap(sampler2D map, vec3 dir)
{
    float phi = atan(dir.y, dir.x);
    float theta = asin(dir.z);
    vec2 uv = vec2(-phi * ONE_OVER_TWO_PI + 0.5f, -theta * ONE_OVER_PI + 0.5f);
    return texture(map, uv).rgb;
}

vec3 screenSpaceUvToViewDir(vec2 uv, mat4 ViewMatrix, float halfVFovRadians, float aspectRatio)
{
    float yHalf = halfVFovRadians;
    float xHalf = yHalf * aspectRatio;

    vec2 ndc = uv;
    ndc = ndc * 2.0f - 1.0f; // normalize to [-1, 1] on both axes
    ndc.y = -ndc.y;

    vec3 camSpaceDir = normalize(vec3(xHalf * ndc.x, yHalf * ndc.y, -1));
    mat3 camToWorldRot = transpose(mat3(ViewMatrix));
    return camToWorldRot * camSpaceDir;
}