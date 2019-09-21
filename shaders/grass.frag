#version 430 core

in vec3 normal;
out vec4 FragColor;

void main() {
  vec3 dir_to_light = vec3(0, 0, 1);
  float align_factor = max(0, dot(dir_to_light, normal));

  vec4 col_dark = vec4(0.494, 0.643, 0.369, 1);
  vec4 col_light = vec4(0.706, 0.851, 0.545, 0.92);

  FragColor = mix(col_dark, col_light, align_factor);
}
