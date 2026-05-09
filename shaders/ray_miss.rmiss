#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 ResultColor;

void main() {
	ResultColor = gl_WorldRayDirectionEXT * 0.5 + 0.5;
}