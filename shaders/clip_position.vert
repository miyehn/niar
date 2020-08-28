#version 330 core

uniform mat4 OBJECT_TO_CLIP;//Camera::Active->world_to_clip() * cube->object_to_world();

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

void main() {
  gl_Position = OBJECT_TO_CLIP * vec4(in_position, 1);
}
