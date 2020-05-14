#version 330 core

uniform sampler2D TEX;

in vec2 vf_uv;
out vec4 FragColor;

void main() {

  FragColor = texture(TEX, vf_uv);

}
