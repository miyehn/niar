#version 330 core

uniform mat4 OBJECT_TO_CLIP;//Camera::Active->world_to_clip() * cube->object_to_world();
uniform mat4 OBJECT_TO_WORLD;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

out vec3 vf_position;

void main() {
  gl_Position = OBJECT_TO_CLIP * vec4(in_position, 1);
	vf_position = (OBJECT_TO_WORLD * vec4(in_position, 1)).xyz;
}
