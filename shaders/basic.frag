#version 330 core

in vec3 vf_normal;
in vec4 vf_color;

out vec4 FragColor;

void main() {
  
  FragColor = vf_color * clamp(dot(normalize(vf_normal), vec3(0, 0, 1)), 0, 1);
}
