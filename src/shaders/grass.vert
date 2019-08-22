#version 410 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in float in_gray;

out float v_gray;

void main() {
  gl_Position = vec4(in_pos, 1.0f);
  v_gray = in_gray;
}
