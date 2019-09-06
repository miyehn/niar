#version 430 core

layout(r32f, binding = 0) uniform readonly image1D comp_o;
out vec4 FragColor;

void main() {
  vec4 data = imageLoad(comp_o, 4);
  float bit = data.x;
  FragColor = vec4(bit, bit, bit, 1.0);
}
