#include "cshared/lights.h"
#include "rt_common.glsl"
#include "gltf_bindless_material.glsl"

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

// Opaque geometry is opaque in the BLASes, so rays commit its hits by themselves and terminates the while loop.
// Only alpha clipped and translucent geometry produces candidate hits enter body of the while loop and get here.

// alpha-clipped materials cut out hard at their threshold as geometry.frag does when rasterizing, and translucent ones
// stop the ray with probability alpha.

// whether the current candidate hit of rq counts as a surface hit. whiteNoise is a per-ray random number in [0, 1)
bool candidateHitPassesAlpha(rayQueryEXT rq, float whiteNoise)
{
    RayHitResult candidate = interpretCandidateRayQuery(rq);

    // which geometry, which material
    GpuSceneInstanceRecord instanceRecord = SceneInstanceRecords[candidate.instanceCustomIndex];
    GpuGeometryRecord geometry = BindlessGeometryRecords[instanceRecord.geometryRecordIndex];
    GpuMaterial material = BindlessMaterials[instanceRecord.bindlessMaterialIndex];

    bool translucent = (instanceRecord.flags & GPU_SCENE_INSTANCE_FLAG_TRANSLUCENT) != 0u;
    float clipThreshold = material.emissiveFactorAndClipThreshold.a;

    vec2 uv;
    reconstructHitUv(candidate, geometry, uv);

    float albedoAlpha = sampleBindlessTexture2DLod(material.textureIndices[GLTF_MATERIAL_TEXTURE_ALBEDO], uv, 0.0).a;
    if (!translucent) {
        return albedoAlpha >= clipThreshold;
    }

    // each candidate gets its own random number, so that layered translucent surfaces are decided independently
    float alpha = albedoAlpha * material.baseColorFactor.a;
    return white_noise01(uvec3(candidate.instanceCustomIndex, candidate.primitiveIndex, 0u), whiteNoise) < alpha;
}

float shadowFactor(accelerationStructureEXT tlas, vec3 worldPos, vec3 normal, vec3 dirToLight, float tMax, float whiteNoise)
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

    // opaque geometry auto-commits; only alpha clipped and translucent geometry generates candidates
    while (rayQueryProceedEXT(rq)) {
        if (candidateHitPassesAlpha(rq, whiteNoise)) {
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
    float whiteNoise,
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
        float shadow = shadowFactor(tlas, worldPos, normal, dirToLight, dist - EPSILON, whiteNoise);

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
        float shadow = shadowFactor(tlas, worldPos, normal, dirToLight, 10000.0, whiteNoise);

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

// radiance leaving a hit surface toward surface.outgoingDirection: emission + direct lighting
vec3 shadeSurface(accelerationStructureEXT tlas, HitSurface surface, float whiteNoise)
{
    vec3 albedo =
        sampleBindlessTexture2DLod(surface.material.textureIndices.x, surface.uv, 0.0).rgb *
        surface.material.baseColorFactor.rgb;
    vec3 emission =
        sampleBindlessTexture2DLod(surface.material.textureIndices.w, surface.uv, 0.0).rgb *
        surface.material.emissiveFactorAndClipThreshold.rgb;
    vec3 orm =
        sampleBindlessTexture2DLod(surface.material.textureIndices.z, surface.uv, 0.0).rgb *
        surface.material.ormAndNormalStrength.rgb;
    orm.g = clamp(orm.g, 0.04, 1.0); // roughness
    orm.b = clamp(orm.b, 0.0, 1.0); // metallic

    return emission + accumulateLighting(
        tlas,
        surface.worldPosition,
        surface.worldNormal,
        albedo,
        orm,
        whiteNoise,
        surface.outgoingDirection);
}
