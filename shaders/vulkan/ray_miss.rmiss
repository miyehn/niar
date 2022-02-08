#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 ResultColor;

void main() {
	ResultColor = vec3(0.412f, 0.796f, 1.0f);
}