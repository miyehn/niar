#version 450 core

layout(location=0) in vec4 vf_position;
layout(location=1) in vec2 vf_uv;
layout(location=2) in mat3 TANGENT_TO_WORLD_ROT;

layout(set = 3, binding = 1) uniform MaterialParamsBufferObject {
    vec4 BaseColorFactor;
    vec4 OcclusionRoughnessMetallicNormalStrengths;
    vec4 _pad0;
    vec4 _pad1;
} materialParams;

layout(set = 3, binding = 2) uniform sampler2D AlbedoMap;
layout(set = 3, binding = 3) uniform sampler2D NormalMap;
layout(set = 3, binding = 4) uniform sampler2D ORMMap;

layout(location=0) out vec4 Position;
layout(location=1) out vec4 Normal;
layout(location=2) out vec4 Color;
layout(location=3) out vec4 ORM;

void main()
{
    vec2 uv = vf_uv;

    vec4 albedoSample = texture(AlbedoMap, uv);
    if (albedoSample.a < 0.5f) discard;

    Color = vec4(albedoSample.rgb * materialParams.BaseColorFactor.rgb, 1);

    Position = vec4(vf_position.xyz, 0);

    vec3 sampled_normal = texture(NormalMap, uv).rgb * 2 - 1.0;
    sampled_normal.rg *= materialParams.OcclusionRoughnessMetallicNormalStrengths.a;
    Normal = vec4(normalize(TANGENT_TO_WORLD_ROT * sampled_normal), 0);

    ORM = vec4(texture(ORMMap, uv).rgb * materialParams.OcclusionRoughnessMetallicNormalStrengths.rgb, 0);
}
