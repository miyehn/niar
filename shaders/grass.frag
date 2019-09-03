#version 410 core

in vec4 col;
out vec4 FragColor;

void main() {
  FragColor = col;// vec4(0.2, 0.5, 0.3, 1);
}
