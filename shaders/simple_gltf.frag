#version 450 core

#include "gltf_bindless_material.glsl"

layout(location=0) out vec4 outColor;

void main() {
    vec2 uv = vf_uv;
    GpuMaterial material = getGltfMaterial();

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
