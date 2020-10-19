#version 330 core

uniform mat3 OBJECT_TO_WORLD_ROT;
uniform mat4 OBJECT_TO_WORLD;
uniform mat4 OBJECT_TO_CLIP;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_tangent;
layout (location = 3) in vec2 in_uv;

out vec4 vf_position;
out vec2 vf_uv;
out mat3 TANGENT_TO_WORLD_ROT;

void main() {
	gl_Position = OBJECT_TO_CLIP * vec4(in_position, 1);
  vf_position = OBJECT_TO_WORLD * vec4(in_position, 1);

  vf_uv = in_uv;

  vec3 N = in_normal;
  vec3 T = OBJECT_TO_WORLD_ROT * in_tangent;
  vec3 B = cross(N, T);
  TANGENT_TO_WORLD_ROT = mat3(T, B, N);
}
