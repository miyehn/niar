#version 330 core

in vec4 vf_position;
in vec3 vf_normal;
in vec3 vf_color;

in float Z;

layout(location=0) out vec3 Position;
layout(location=1) out vec3 Normal;
layout(location=2) out vec3 Color;

void main() {
	Position = vf_position.xyz;

	Normal = normalize(vf_normal / Z);
	Color = vf_color / Z;
}
