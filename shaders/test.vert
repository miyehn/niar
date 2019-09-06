#version 430 core

layout (location = 0) in vec3 in_data;
uniform mat4 transformation;

void main() {
  gl_Position = transformation * vec4(in_data, 1.0);
}
