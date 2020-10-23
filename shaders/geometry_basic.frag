#version 330 core

in vec4 vf_position;
in vec3 vf_normal;
in vec2 vf_uv;

uniform vec3 Tint;
uniform sampler2D BaseColor;
uniform sampler2D NormalMap;

layout(location=0) out vec3 Position;
layout(location=1) out vec3 Normal;
layout(location=2) out vec3 Color;
layout(location=3) out vec3 MRA;

void main()
{
	vec2 uv = vf_uv;

	Position = vf_position.xyz;
	Normal = normalize(vf_normal);
	Color = texture(BaseColor, uv).rgb * Tint;
	MRA = vec3(0, 1, 1);
}