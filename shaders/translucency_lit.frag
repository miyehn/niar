#version 450 core

#include "utils.glsl"
#include "scene_common.glsl"
#include "lighting_common.glsl"

#include "gltf_vertex_out_material_params.glsl"

layout(location=0) out vec4 outColor;

void main() {
    vec2 uv = vf_uv;

    vec4 albedoSample = texture(AlbedoMap, uv);
    vec4 baseColor = albedoSample * materialParams.BaseColorFactor;

    vec3 normal = texture(NormalMap, uv).rgb * 2 - 1.0f;
    normal.rg *= materialParams.OcclusionRoughnessMetallicNormalStrengths.a;
    normal = normalize(TANGENT_TO_WORLD_ROT * normal);

    vec3 orm = texture(ORMMap, uv).rgb * materialParams.OcclusionRoughnessMetallicNormalStrengths.rgb;

    vec3 emission = materialParams.EmissiveFactorClipThreshold.rgb * texture(EmissiveMap, uv).rgb;

    vec3 litResult = emission + accumulateLighting(
        vf_position.xyz + GetViewInfo().CameraPosition,
        normal,
        baseColor.rgb,
        orm
    );

    outColor = vec4(litResult, baseColor.a);
}
