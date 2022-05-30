#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 ResultColor;

hitAttributeEXT vec2 HitAttribs;

void main() {
	const vec3 barycentrics = vec3(HitAttribs.x, HitAttribs.y, 0);//vec3(1.0f - HitAttribs.x - HitAttribs.y, HitAttribs.x, HitAttribs.y);
	ResultColor = vec3(barycentrics);
}