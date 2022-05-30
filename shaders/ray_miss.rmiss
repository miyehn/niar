#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 ResultColor;

void main() {
	ResultColor = vec3(0.2, 0.3, 0.4);
}