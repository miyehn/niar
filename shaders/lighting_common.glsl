#include "cshared/lights.h"
#include "rt_common.glsl"
#include "gltf_bindless_material.glsl"

// A shader that declares sampler2D EnvironmentMap and includes sky_common.glsl before this file should define
// BACKGROUND_AVAILABLE as 1 first, so that rays escaping the scene see the sky atmosphere or the environment map
// (per ViewInfo.BackgroundOption). Without it they see black.
#ifndef BACKGROUND_AVAILABLE
#define BACKGROUND_AVAILABLE 0
#endif

vec3 fresnelSchlick(float VdotH, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1 - VdotH, 5.0);
}

// gives the specular highlight
float distributionFn(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = dot(N, H);
    float NdotH2 = NdotH * NdotH;

    // denomenator
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return a2 / denom;
}

float geometrySub(float NdotV, float roughness)
{
    float r = roughness + 1;
    float k = r * r / 8.0; // IBL needs something else here
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / denom;
}

float geometrySmith(float NdotL, float NdotV, float roughness)
{
    float g1 = geometrySub(NdotV, roughness);
    float g2 = geometrySub(NdotL, roughness);
    return g1 * g2;
}

float shadowFactor(accelerationStructureEXT tlas, vec3 worldPos, vec3 normal, vec3 dirToLight, float tMax, float whiteNoiseXYF, uint lightIndex)
{
    const vec3 rayOrigin = worldPos + normal * 0.005;

    rayQueryEXT rq;
    rayQueryInitializeEXT(
        rq,
        tlas,
        gl_RayFlagsTerminateOnFirstHitEXT,
        0xFF, // cull mask
        rayOrigin,
        0, // tmin
        dirToLight,
        tMax);

    // unique per light only: once a pixel shades more than one vertex of a path, this needs noise that is unique per bounce
    float whiteNoiseXYFR = makeWhiteNoiseXYFR(whiteNoiseXYF, NOISE_KIND_SHADOW_RAY, lightIndex);
    // opaque geometry auto-commits; only alpha clipped and translucent geometry generates candidates
    while (rayQueryProceedEXT(rq)) {
        if (candidateHitPassesAlpha(rq, whiteNoiseXYFR)) {
            rayQueryConfirmIntersectionEXT(rq);
        }
    }

    return rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionNoneEXT ? 1.0 : 0.0;
}

vec3 evaluateLight(
    vec3 albedo,
    float metallic,
    vec3 normal,
    float roughness,
    vec3 outgoingDirection,
    vec3 dirToLight,
    vec3 radiance,
    float visibility)
{
    float NdotL = max(dot(normal, dirToLight), 0);
    float NdotV = max(0, dot(normal, outgoingDirection));
    if (NdotL <= 0.0 || NdotV <= 0.0 || visibility <= 0.0) {
        return vec3(0.0);
    }

    vec3 halfVec = normalize(outgoingDirection + dirToLight);

    // Fresnel
    vec3 F0 = vec3(0.04); // base reflectivity for non-metals
    F0 = mix(F0, albedo, metallic); // if metal, use what's in albedo map for base reflectivity
    vec3 F = fresnelSchlick(max(dot(halfVec, outgoingDirection), 0), F0);

    // Distribution
    float D = distributionFn(normal, halfVec, roughness);

    // Geometry
    float G = geometrySmith(NdotL, NdotV, roughness);

    // specular (cook tolerance)
    vec3 num = F * D * G;
    float denom = 4.0 * NdotV * NdotL;
    vec3 specular = num / max(denom, 0.001);

    vec3 kSpecular = F;
    vec3 kDiffuse = vec3(1.0) - kSpecular;
    kDiffuse *= 1.0 - metallic;
    vec3 diffuse = kDiffuse * albedo / PI;

    return (diffuse + specular) * NdotL * radiance * visibility;
}

vec3 accumulateLighting(
    accelerationStructureEXT tlas,
    vec3 worldPos,
    vec3 normal,
    vec3 albedo,
    vec3 orm,
    float whiteNoiseXYF,
    vec3 outgoingDirection)
{
    float metallic = orm.b;
    float roughness = orm.g;
    vec3 normalizedOutgoingDirection = normalize(outgoingDirection);

    vec3 result = vec3(0, 0, 0);
    ViewInfo viewInfo = GetViewInfo();

    // point lights
    for (int i = 0; i < viewInfo.NumPointLights; i++)
    {
        // some useful properties
        vec3 toLight = PointLights.Data[i].position - worldPos;
        float dist = length(toLight);
        if (dist <= EPSILON) {
            continue;
        }
        vec3 dirToLight = toLight / dist;
        float atten = 1.0 / dot(toLight, toLight);
        vec3 radiance = PointLights.Data[i].color * atten;
        float shadow = shadowFactor(tlas, worldPos, normal, dirToLight, dist - EPSILON, whiteNoiseXYF, uint(i));

        result += evaluateLight(
            albedo,
            metallic,
            normal,
            roughness,
            normalizedOutgoingDirection,
            dirToLight,
            radiance,
            shadow);
    }

    // directional lights
    for (int i = 0; i < viewInfo.NumDirectionalLights; i++)
    {
        vec3 lightDir = DirectionalLights.Data[i].direction;
        vec3 dirToLight = normalize(-lightDir);
        vec3 radiance = DirectionalLights.Data[i].color;
        float shadow = shadowFactor(tlas, worldPos, normal, dirToLight, 10000.0, whiteNoiseXYF, uint(viewInfo.NumPointLights + i));

        result += evaluateLight(
            albedo,
            metallic,
            normal,
            roughness,
            normalizedOutgoingDirection,
            dirToLight,
            radiance,
            shadow);
    }

    return result;
}

// radiance leaving a committed hit toward the ray's origin: emission + direct lighting. A hit that can't be
// reconstructed shows as magenta (kept dim, since GI accumulates these)
vec3 shadeCommittedHit(accelerationStructureEXT tlas, RayHitResult hitResult, vec3 rayOrigin, vec3 rayDir, float whiteNoiseXYF)
{
    const vec3 invalidColor = vec3(1, 0, 1) * 0.05;

    if (hitResult.instanceCustomIndex == ~0u) {
        return invalidColor;
    }

    GpuSceneInstanceRecord sceneInstance = SceneInstanceRecords[hitResult.instanceCustomIndex];

    HitSurface surface;
    if (!reconstructHitSurface(hitResult, sceneInstance, rayOrigin, rayDir, surface)) {
        return invalidColor;
    }

    SurfaceMaterial material = fetchSurfaceMaterial(surface);

    return material.emission + accumulateLighting(
        tlas,
        surface.worldPosition,
        surface.worldNormal,
        material.albedo,
        material.orm,
        whiteNoiseXYF,
        surface.outgoingDirection);
}

// radiance arriving along a ray that is not a shadow ray (camera ray, GI ray): the shaded surface it hits, or the
// background where it escapes the scene
vec3 traceRadiance(accelerationStructureEXT tlas, vec3 origin, vec3 dir, float whiteNoiseXYF)
{
    float whiteNoiseXYFR = makeWhiteNoiseXYFR(whiteNoiseXYF, NOISE_KIND_PATH_RAY, 0u);
    RayHitResult hitResult = traceClosestHit(tlas, origin, dir, whiteNoiseXYFR);
    if (hitResult.intersectionType != RAY_HIT_NONE) {
        return shadeCommittedHit(tlas, hitResult, origin, dir, whiteNoiseXYF);
    }

#if BACKGROUND_AVAILABLE
    ViewInfo viewInfo = GetViewInfo();
    if (viewInfo.BackgroundOption == 1) {
        return sampleLongLatMap(EnvironmentMap, dir, 0.0);
    }
    if (viewInfo.BackgroundOption == 2) {
        return sampleSkyAtmosphere(dir);
    }
#endif
    return vec3(0.0);
}
