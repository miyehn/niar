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

layout(location=0) out vec4 Normal;
layout(location=1) out vec4 Color;
layout(location=2) out vec4 ORM;
layout(location=3) out vec2 MotionVector;

void main()
{
    vec2 uv = vf_uv;
    GpuMaterial material = getGltfMaterial(pc.MaterialIndex);

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

    vec2 currentUV = (vf_currentClipPos.xy / vf_currentClipPos.w) * 0.5 + 0.5;
    if (vf_prevClipPos.w <= 0.0) {
        MotionVector = vec2(2.0, 0.0); // out-of-range sentinel; bounds check in GI will reject this history sample
    } else {
        vec2 prevUV = (vf_prevClipPos.xy / vf_prevClipPos.w) * 0.5 + 0.5;
        MotionVector = currentUV - prevUV;
    }
}
