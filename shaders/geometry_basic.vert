#version 330 core

uniform mat3 OBJECT_TO_WORLD_ROT;
uniform mat4 OBJECT_TO_WORLD;
uniform mat4 OBJECT_TO_CLIP;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_tangent;
layout (location = 3) in vec2 in_uv;

out vec4 vf_position;
out vec3 vf_normal;
out vec2 vf_uv;

void main() {
	gl_Position = OBJECT_TO_CLIP * vec4(in_position, 1);
  vf_position = OBJECT_TO_WORLD * vec4(in_position, 1);

	// perspective-correct lerp: http://15462.courses.cs.cmu.edu/spring2019/lecture/texture/slide_033
  vf_normal = OBJECT_TO_WORLD_ROT * in_normal;

  vf_uv = in_uv;
}
