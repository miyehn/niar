#version 330 core

uniform sampler2D TEX;

in vec2 vf_uv;
out vec4 FragColor;

void main() {

  vec4 gamma = vec4( vec3(0.455), 1 );
  FragColor = pow(texture(TEX, vf_uv), gamma);

}
