#version 450 core

layout(location=0) in vec4 vf_position;
layout(location=1) in vec2 vf_uv;
layout(location=2) in mat3 TANGENT_TO_WORLD_ROT;

layout(set = 3, binding = 1) uniform MaterialParamsBufferObject {
    vec4 BaseColorFactor;
    vec4 OcclusionRoughnessMetallicNormalStrengths;
    vec4 EmissiveFactorClipThreshold;
    vec4 _pad0;
} materialParams;

layout(set = 3, binding = 2) uniform sampler2D AlbedoMap;
layout(set = 3, binding = 3) uniform sampler2D NormalMap;
layout(set = 3, binding = 4) uniform sampler2D ORMMap;

layout(location=0) out vec4 outColor;

void main() {
    vec2 uv = vf_uv;

    vec4 albedoSample = texture(AlbedoMap, uv);
    if (albedoSample.a <= materialParams.EmissiveFactorClipThreshold.a) discard;

    vec3 color = albedoSample.rgb * materialParams.BaseColorFactor.rgb;

    vec3 sampledNormal = texture(NormalMap, uv).rgb * 2 - 1.0;
    sampledNormal.rg = -sampledNormal.rg;
    vec3 normal = TANGENT_TO_WORLD_ROT * normalize(sampledNormal);

    vec3 lightDir = normalize(vec3(0, 0.5, 1));
    float brightness = clamp(dot(normal, lightDir), 0, 1);

    outColor = vec4(color * mix(0.25f, 1.0f, brightness), 1);
}