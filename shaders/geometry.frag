#version 450 core

#include "gltf_vertex_out_material_params.glsl"

layout(location=0) out vec4 Position;
layout(location=1) out vec4 Normal;
layout(location=2) out vec4 Color;
layout(location=3) out vec4 ORM;

void main()
{
    vec2 uv = vf_uv;

    vec4 albedoSample = texture(AlbedoMap, uv);
    if (albedoSample.a <= materialParams.EmissiveFactorClipThreshold.a) discard;

    Color = vec4(albedoSample.rgb * materialParams.BaseColorFactor.rgb, 1);

    vec3 emission = materialParams.EmissiveFactorClipThreshold.rgb * texture(EmissiveMap, uv).rgb;

    Position = vec4(vf_position.xyz, emission.r);

    vec3 sampled_normal = texture(NormalMap, uv).rgb * 2 - 1.0;
    sampled_normal.rg *= materialParams.OcclusionRoughnessMetallicNormalStrengths.a;
    Normal = vec4(normalize(TANGENT_TO_WORLD_ROT * sampled_normal), emission.g);

    ORM = vec4(texture(ORMMap, uv).rgb * materialParams.OcclusionRoughnessMetallicNormalStrengths.rgb, emission.b);
}
