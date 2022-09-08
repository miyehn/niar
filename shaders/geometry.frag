#version 450 core

layout(location=0) in vec4 vf_position;
layout(location=1) in vec2 vf_uv;
layout(location=2) in mat3 TANGENT_TO_WORLD_ROT;

layout(set = 3, binding = 1) uniform MaterialParamsBufferObject {
    vec4 BaseColorFactor;
    vec4 MetallicRoughnessAONormalStrengths;
    vec4 _pad0;
    vec4 _pad1;
} materialParams;

layout(set = 3, binding = 2) uniform sampler2D AlbedoMap;
layout(set = 3, binding = 3) uniform sampler2D NormalMap;
layout(set = 3, binding = 4) uniform sampler2D MetallicRoughnessMap;
layout(set = 3, binding = 5) uniform sampler2D AOMap;

layout(location=0) out vec4 Position;
layout(location=1) out vec4 Normal;
layout(location=2) out vec4 Color;
layout(location=3) out vec4 MRAV;

void main()
{
    vec2 uv = vf_uv;

    Position = vec4(vf_position.xyz, 0);

    vec3 sampled_normal = texture(NormalMap, uv).rgb * 2 - 1.0;
    sampled_normal.rg = -sampled_normal.rg;
    sampled_normal.rg *= materialParams.MetallicRoughnessAONormalStrengths.a;
    Normal = vec4(TANGENT_TO_WORLD_ROT * normalize(sampled_normal), 0);

    Color = vec4(texture(AlbedoMap, uv).rgb * materialParams.BaseColorFactor.rgb, 0);

    vec2 metallic_roughness = texture(MetallicRoughnessMap, uv).rg;
    float metallic = metallic_roughness.r * materialParams.MetallicRoughnessAONormalStrengths.r;
    float roughness = metallic_roughness.g * materialParams.MetallicRoughnessAONormalStrengths.g;
    float ao = texture(AOMap, uv).r * materialParams.MetallicRoughnessAONormalStrengths.b;
    MRAV = vec4(metallic, roughness, ao, 1);
}
