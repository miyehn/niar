#version 430 core

layout (location = 0) in vec4 in_data;

out vec4 v_data;

void main() {
  v_data = in_data;
}
