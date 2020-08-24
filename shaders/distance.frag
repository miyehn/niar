#version 330 core

uniform vec3 FIXED_POINT;

in vec3 vf_position;
out vec4 FragColor;

void main() {
	float d = length(vf_position - FIXED_POINT);
	FragColor = vec4(d, 0, 0, 0);
}
