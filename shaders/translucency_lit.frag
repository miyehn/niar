#version 460 core
#extension GL_EXT_ray_query : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require

#include "utils.glsl"
#include "scene_common.glsl"
#include "cshared/lights.h"

#include "gltf_bindless_material.glsl"

layout(set = 0, binding = 5) uniform PointLightsInfo {
    PointLightInfo Data[MAX_LIGHTS_PER_PASS];
} PointLights;
layout(set = 0, binding = 6) uniform DirectionalLightsInfo {
    DirectionalLightInfo Data[MAX_LIGHTS_PER_PASS];
} DirectionalLights;
layout(set = 0, binding = 7) uniform sampler2D EnvironmentMap;
layout(set = 0, binding = 8) uniform accelerationStructureEXT SceneTLAS;
layout(set = 0, binding = 9) uniform sampler2D IndirectLighting;

#include "lighting_common.glsl"

layout(push_constant) uniform GltfFragmentPushConstants {
    layout(offset = GLTF_MATERIAL_INDEX_PUSH_OFFSET) uint MaterialIndex;
} pc;

layout(location=0) in vec4 vf_position;
layout(location=1) in vec2 vf_uv;
layout(location=2) in vec4 vf_currentClipPos;
layout(location=3) in vec4 vf_prevClipPos;
layout(location=4) in mat3 TANGENT_TO_WORLD_ROT;

layout(location=0) out vec4 outColor;

void main() {
    vec2 uv = vf_uv;
    GpuMaterial material = getGltfMaterial(pc.MaterialIndex);

    vec4 albedoSample = sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_ALBEDO, uv);
    vec4 baseColor = albedoSample * material.baseColorFactor;

    vec3 normal = sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_NORMAL, uv).rgb * 2 - 1.0f;
    normal.rg *= material.ormAndNormalStrength.a;
    normal = normalize(TANGENT_TO_WORLD_ROT * normal);

    vec3 orm = sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_ORM, uv).rgb * material.ormAndNormalStrength.rgb;

    vec3 emission = material.emissiveFactorAndClipThreshold.rgb *
        sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_EMISSIVE, uv).rgb;
    ViewInfo viewInfo = GetViewInfo();
    vec3 worldPos = vf_position.xyz + viewInfo.CameraPosition;

    vec3 litResult = emission + accumulateLighting(
        SceneTLAS,
        worldPos,
        normal,
        baseColor.rgb,
        orm,
        normalize(viewInfo.CameraPosition - worldPos)
    );
    /*
    todo [myn]: indirect for translucent surfaces
    vec2 screenUv = gl_FragCoord.xy / GetViewInfo().RenderSize;
    vec3 indirectDiffuse = texture(IndirectLighting, screenUv).rgb * baseColor.rgb;
    litResult += indirectDiffuse;
    */

    outColor = vec4(litResult, baseColor.a);
}
