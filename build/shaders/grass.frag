#version 410 core

in float e_gray;
out vec4 FragColor;

void main() {
  FragColor = vec4(e_gray, e_gray, e_gray, 1.0);
}
