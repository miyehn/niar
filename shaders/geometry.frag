#version 330 core

in vec4 vf_position;
in vec2 vf_uv;
in mat3 TANGENT_TO_WORLD_ROT;

uniform vec3 Tint;
uniform sampler2D BaseColor;
uniform sampler2D NormalMap;

layout(location=0) out vec3 Position;
layout(location=1) out vec3 Normal;
layout(location=2) out vec3 Color;

void main()
{
	vec2 uv = vf_uv;

	Position = vf_position.xyz;
	vec3 sampled_normal = texture(NormalMap, uv).rgb * 2 - 1.0;
	sampled_normal.rg = -sampled_normal.rg;
	Normal = TANGENT_TO_WORLD_ROT * normalize(sampled_normal);
	Color = texture(BaseColor, uv).rgb * Tint;
}