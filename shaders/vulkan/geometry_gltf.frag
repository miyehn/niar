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

layout(location=0) out vec3 Position;
layout(location=1) out vec3 Normal;
layout(location=2) out vec3 Color;
layout(location=3) out vec3 MRA;

void main()
{
    vec2 uv = vf_uv;

    Position = vf_position.xyz;
    vec3 sampled_normal = texture(NormalMap, uv).rgb * 2 - 1.0;
    sampled_normal.rg = -sampled_normal.rg;
    Normal = TANGENT_TO_WORLD_ROT * normalize(sampled_normal);
    Color = texture(AlbedoMap, uv).rgb;

    vec2 metallic_roughness = texture(MetallicRoughnessMap, uv).rg;
    float ao = texture(AOMap, uv).r;
    MRA = vec3(metallic_roughness.r, metallic_roughness.g, ao);
}
