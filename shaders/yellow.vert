#version 330 core

uniform mat4 WORLD_TO_CLIP;

layout (location = 0) in vec3 in_position;

void main() {
  gl_Position = WORLD_TO_CLIP * vec4(in_position, 1);
}
