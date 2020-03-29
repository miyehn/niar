#version 330 core

uniform sampler2D TEX;

in vec2 vf_uv;
out vec4 FragColor;

void main() {

  float u = vf_uv.x;
  float v = vf_uv.y;
  FragColor = texture(TEX, vf_uv);

}
