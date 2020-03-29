#version 330 core

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec2 in_uv;

out vec2 vf_uv;

void main() {
  vf_uv = in_uv;
  gl_Position = vec4(in_position, 0, 1);
}
