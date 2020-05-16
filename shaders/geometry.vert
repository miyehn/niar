#version 330 core

const int MaxShadowCastingLights = 2;

uniform mat3 OBJECT_TO_WORLD_ROT;
uniform mat4 OBJECT_TO_WORLD;
uniform mat4 OBJECT_TO_CLIP;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec4 in_color;

out vec4 vf_position;
out vec3 vf_normal;
out vec3 vf_color;

void main() {
	gl_Position = OBJECT_TO_CLIP * vec4(in_position, 1);
  vf_position = OBJECT_TO_WORLD * vec4(in_position, 1);
  vf_normal = OBJECT_TO_WORLD_ROT * in_normal;
  vf_color = in_color.rgb;
}
