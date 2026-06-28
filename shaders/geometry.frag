#version 450 core

#include "gltf_bindless_material.glsl"

layout(location=0) out vec4 Normal;
layout(location=1) out vec4 Color;
layout(location=2) out vec4 ORM;

void main()
{
    vec2 uv = vf_uv;
    GpuMaterial material = getGltfMaterial();

    vec4 albedoSample = sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_ALBEDO, uv);
    if (albedoSample.a <= material.emissiveFactorAndClipThreshold.a) discard;

    vec3 emission = material.emissiveFactorAndClipThreshold.rgb *
        sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_EMISSIVE, uv).rgb;

    Color = vec4(albedoSample.rgb * material.baseColorFactor.rgb, emission.r);

    vec3 sampled_normal = sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_NORMAL, uv).rgb * 2 - 1.0;
    sampled_normal.rg *= material.ormAndNormalStrength.a;
    Normal = vec4(normalize(TANGENT_TO_WORLD_ROT * sampled_normal), emission.g);

    ORM = vec4(
        sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_ORM, uv).rgb * material.ormAndNormalStrength.rgb,
        emission.b);
}
