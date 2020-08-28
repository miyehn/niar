#version 330 core

const int MaxLights = 6;

struct PointLight {
	vec3 Position;
	samplerCube ShadowMap;
};

uniform mat4 OBJECT_TO_CLIP;
uniform mat4 OBJECT_TO_WORLD;
uniform mat3 OBJECT_TO_WORLD_ROT;

uniform int NumPointLights;
uniform PointLight PointLights[MaxLights];

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

out vec3 vf_position;
out vec3 vf_normal;

void main() {
	gl_Position = OBJECT_TO_CLIP * vec4(in_position, 1);
  vf_position = (OBJECT_TO_WORLD * vec4(in_position, 1)).xyz;
  vf_normal = OBJECT_TO_WORLD_ROT * in_normal;
}
