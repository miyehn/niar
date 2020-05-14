#version 330 core

in vec4 vf_position;
in vec3 vf_normal;
in vec4 vf_color;

layout(location=0) out vec4 Position;
layout(location=1) out vec4 Normal;
layout(location=2) out vec4 Color;

void main() {
	Position = vf_position;
	Normal = vec4(vf_normal, 1);
	Color = vf_color;
}
