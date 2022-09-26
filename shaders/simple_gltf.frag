#version 450 core

#include "gltf_vertex_out_material_params.glsl"

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