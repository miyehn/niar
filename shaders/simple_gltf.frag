#version 450 core

#include "gltf_bindless_material.glsl"

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
    if (albedoSample.a <= material.emissiveFactorAndClipThreshold.a) discard;

    vec3 color = albedoSample.rgb * material.baseColorFactor.rgb;

    vec3 sampledNormal = sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_NORMAL, uv).rgb * 2 - 1.0;
    sampledNormal.rg = -sampledNormal.rg;
    vec3 normal = TANGENT_TO_WORLD_ROT * normalize(sampledNormal);

    vec3 lightDir = normalize(vec3(0, 0.5, 1));
    float brightness = clamp(dot(normal, lightDir), 0, 1);

    outColor = vec4(color * mix(0.25f, 1.0f, brightness), 1);
}
