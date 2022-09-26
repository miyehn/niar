#version 450 core

#include "utils.glsl"
#include "scene_common.glsl"
#include "lighting_common.glsl"

layout(location=0) in vec4 vf_position;
layout(location=1) in vec2 vf_uv;
layout(location=2) in mat3 TANGENT_TO_WORLD_ROT;

layout(set = 3, binding = 1) uniform MaterialParamsBufferObject {
    vec4 BaseColorFactor;
    vec4 OcclusionRoughnessMetallicNormalStrengths;
    vec4 ClipThreshold_pad0;
    vec4 _pad1;
} materialParams;

layout(set = 3, binding = 2) uniform sampler2D AlbedoMap;
layout(set = 3, binding = 3) uniform sampler2D NormalMap;
layout(set = 3, binding = 4) uniform sampler2D ORMMap;

layout(location=0) out vec4 outColor;

void main() {
    vec2 uv = vf_uv;

    vec4 albedoSample = texture(AlbedoMap, uv);
    vec4 baseColor = albedoSample * materialParams.BaseColorFactor;

    vec3 normal = texture(NormalMap, uv).rgb * 2 - 1.0f;
    normal.rg *= materialParams.OcclusionRoughnessMetallicNormalStrengths.a;
    normal = normalize(TANGENT_TO_WORLD_ROT * normal);

    vec3 orm = texture(ORMMap, uv).rgb * materialParams.OcclusionRoughnessMetallicNormalStrengths.rgb;

    vec3 litResult = accumulateLighting(
        vf_position.xyz + GetViewInfo().CameraPosition,
        normal,
        baseColor.rgb,
        orm
    );

    outColor = vec4(litResult, baseColor.a);
}
