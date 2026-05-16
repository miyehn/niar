#version 460 core
#extension GL_EXT_ray_query : require

#include "utils.glsl"
#include "scene_common.glsl" // (set 0, binding 0; 5-7) frameglobal
#include "lighting_common.glsl" // pbr lighting functions + struct/light UBO bindings
#include "rendertargets.glsl" // (set 0, bindings 1-4) g buffers

#include "sky_common.glsl" // set 1, bindings 0-2; 8-9

layout (set = 0, binding = 7) uniform sampler2D EnvironmentMap;
layout (set = 0, binding = 8) uniform accelerationStructureEXT SceneTLAS;

layout(location = 0) in vec2 vf_uv;

layout(location = 0) out vec4 FragColor;

// Returns 1.0 if the point is unoccluded toward dirToLight, 0.0 if in shadow.
float shadowFactor(vec3 worldPos, vec3 normal, vec3 dirToLight, float tMax)
{
    rayQueryEXT rq;
    rayQueryInitializeEXT(
        rq,
        SceneTLAS,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsOpaqueEXT,
        0xFF,
        worldPos + normal * 0.001, // bias to avoid self-intersection
        0.0,
        dirToLight,
        tMax);
    rayQueryProceedEXT(rq);
    return rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionNoneEXT
        ? 1.0
        : 0.0;
}

// Shadow-aware version of accumulateLighting from lighting_common.glsl.
// Uses the same PBR math but multiplies each light's contribution by a shadow factor.
vec3 accumulateLightingWithShadows(vec3 worldPos, vec3 normal, vec3 albedo, vec3 orm)
{
    ViewInfo viewInfo = GetViewInfo();

    MaterialLightingInfo info;
    info.albedo = albedo;
    info.metallic = orm.b;
    info.normal = normal;
    info.roughness = orm.g;
    info.dirToCam = normalize(viewInfo.CameraPosition - worldPos);
    info.NdotV = max(0, dot(normal, info.dirToCam));

    vec3 result = vec3(0, 0, 0);

    // point lights
    for (int i = 0; i < viewInfo.NumPointLights; i++)
    {
        vec3 toLight = PointLights.Data[i].position - worldPos;
        float dist = length(toLight);
        float atten = 1.0 / dot(toLight, toLight);
        vec3 dirToLight = toLight / dist;
        vec3 halfVec = normalize(info.dirToCam + dirToLight);
        float NdotL = max(dot(normal, dirToLight), 0);
        vec3 radiance = PointLights.Data[i].color * atten;

        info.halfVec = halfVec;
        info.NdotL = NdotL;

        float shadow = shadowFactor(worldPos, normal, dirToLight, dist - 0.01);
        result += lightingContrib(info) * radiance * shadow;
    }

    // directional lights
    for (int i = 0; i < viewInfo.NumDirectionalLights; i++)
    {
        vec3 lightDir = DirectionalLights.Data[i].direction;
        vec3 dirToLight = normalize(-lightDir);
        vec3 halfVec = normalize(info.dirToCam + dirToLight);
        float NdotL = max(dot(normal, dirToLight), 0);
        vec3 radiance = DirectionalLights.Data[i].color;

        info.halfVec = halfVec;
        info.NdotL = NdotL;

        float shadow = shadowFactor(worldPos, normal, dirToLight, 10000.0);
        result += lightingContrib(info) * radiance * shadow;
    }

    return result;
}

void main() {

    FragColor = vec4(0, 0, 0, 1);

    vec4 GPosition = subpassLoad(GBUF0);
    vec4 GNormal = subpassLoad(GBUF1);
    vec4 GColor = subpassLoad(GBUF2);
    vec4 GORM = subpassLoad(GBUF3);

    ViewInfo viewInfo = GetViewInfo();

    float visibility = GColor.a;
    if (visibility > 0.5f) {
        // emission
        FragColor.rgb += vec3(GPosition.a, GNormal.a, GORM.a);
        // the rest of lighting
        FragColor.rgb += accumulateLightingWithShadows(
            GPosition.xyz + viewInfo.CameraPosition,
            GNormal.xyz,
            GColor.rgb,
            GORM.rgb
        );
    }
    else if (viewInfo.BackgroundOption > 0)
    {
        // environment map
        vec3 viewDirWS = screenSpaceUvToViewDir(vf_uv, viewInfo.ViewMatrix, viewInfo.HalfVFovRadians, viewInfo.AspectRatio);
        if (viewInfo.BackgroundOption == 1) {
            FragColor.rgb += sampleLongLatMap(EnvironmentMap, viewDirWS, 0);
        } else if (viewInfo.BackgroundOption == 2) {
            FragColor.rgb += sampleSkyAtmosphere(viewDirWS);
        }
    }
}
