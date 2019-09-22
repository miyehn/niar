#version 430 core

in vec3 normal;
out vec4 FragColor;

void main() {
  vec3 dir_to_light = vec3(0, 0, 1);
  float align_factor = max(0, dot(dir_to_light, gl_FrontFacing ? -normal : normal));

  vec4 col_dark = vec4(0.31, 0.388, 0.137, 1);
  // vec4 col_light = vec4(0.706, 0.851, 0.545, 0.92);
  vec4 col_light = vec4(1,1,1,1);

  FragColor = mix(col_dark, col_light, align_factor);
}
