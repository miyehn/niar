#version 330 core

const int MaxLights = 6;

struct DirectionalLight {
	mat4 OBJECT_TO_CLIP; // vertex
	sampler2D ShadowMap;
	vec3 Direction;
};

uniform mat4 OBJECT_TO_CLIP;
uniform mat4 OBJECT_TO_WORLD;
uniform mat3 OBJECT_TO_WORLD_ROT;

uniform int NumDirectionalLights;
uniform DirectionalLight DirectionalLights[MaxLights];

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec4 in_color;

out vec3 vf_position;
out vec3 vf_normal;
out vec4 vf_DirectionalLightSpacePositions[MaxLights];

void main() {
	gl_Position = OBJECT_TO_CLIP * vec4(in_position, 1);
  vf_position = (OBJECT_TO_WORLD * vec4(in_position, 1)).xyz;
  vf_normal = OBJECT_TO_WORLD_ROT * in_normal;
	for (int i=0; i<NumDirectionalLights; i++) {
		vf_DirectionalLightSpacePositions[i] = DirectionalLights[i].OBJECT_TO_CLIP * vec4(in_position, 1);
	}
}
