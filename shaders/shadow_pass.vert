#version 330 core

const int MaxShadowCastingLights = 2;

struct LightInfo {
	mat4 OBJECT_TO_CLIP; // vertex
	sampler2D ShadowMap;
	vec3 Position;
	bool Directional;
	vec3 Direction;
};

struct DirectionalLight {
	sampler2D ShadowMap;
	mat4 OBJECT_TO_CLIP; // vertex
	vec3 Direction;
};

struct PointLight {
	samplerCube ShadowMap;
	vec3 Position;
};

uniform mat4 OBJECT_TO_CLIP;
uniform mat4 OBJECT_TO_WORLD;
uniform mat3 OBJECT_TO_WORLD_ROT;
uniform LightInfo LightInfos[MaxShadowCastingLights];

uniform int NumDirectionalLights;
uniform int NumPointLights;

uniform DirectionalLight DirectionalLights[MaxShadowCastingLights];
uniform PointLight PointLights[MaxShadowCastingLights];

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec4 in_color;

out vec3 vf_position;
out vec3 vf_normal;
out vec4 vf_PositionLights[MaxShadowCastingLights];

void main() {
	gl_Position = OBJECT_TO_CLIP * vec4(in_position, 1);
  vf_position = (OBJECT_TO_WORLD * vec4(in_position, 1)).xyz;
  vf_normal = OBJECT_TO_WORLD_ROT * in_normal;
	for (int i=0; i<MaxShadowCastingLights; i++) {
		vf_PositionLights[i] = LightInfos[i].OBJECT_TO_CLIP * vec4(in_position, 1);
	}
}
