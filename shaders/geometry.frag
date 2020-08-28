#version 330 core

in vec4 vf_position;
in vec3 vf_normal;
in vec2 vf_uv;

in float Z;

uniform vec3 Tint;
uniform sampler2D BaseColor;

layout(location=0) out vec3 Position;
layout(location=1) out vec3 Normal;
layout(location=2) out vec3 Color;

void main() {
	Position = vf_position.xyz;

	Normal = normalize(vf_normal);

	vec2 uv = vf_uv;
	Color = texture(BaseColor, uv).rgb * Tint;
}
