#version 460 core
#extension GL_EXT_ray_query : require

#include "utils.glsl"
#include "scene_common.glsl"
#include "lighting_common.glsl"

#include "gltf_bindless_material.glsl"

layout(location=0) out vec4 outColor;

void main() {
    vec2 uv = vf_uv;
    GpuMaterial material = getGltfMaterial();

    vec4 albedoSample = sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_ALBEDO, uv);
    vec4 baseColor = albedoSample * material.baseColorFactor;

    vec3 normal = sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_NORMAL, uv).rgb * 2 - 1.0f;
    normal.rg *= material.ormAndNormalStrength.a;
    normal = normalize(TANGENT_TO_WORLD_ROT * normal);

    vec3 orm = sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_ORM, uv).rgb * material.ormAndNormalStrength.rgb;

    vec3 emission = material.emissiveFactorAndClipThreshold.rgb *
        sampleGltfMaterialTexture(material, GLTF_MATERIAL_TEXTURE_EMISSIVE, uv).rgb;

    vec3 litResult = emission + accumulateLighting(
        vf_position.xyz + GetViewInfo().CameraPosition,
        normal,
        baseColor.rgb,
        orm
    );
    vec2 screenUv = gl_FragCoord.xy / GetViewInfo().RenderSize;
    litResult += sampleIndirectLighting(screenUv);

    outColor = vec4(litResult, baseColor.a);
}
