#version 430 core

in vec3 normal;
out vec4 FragColor;

float map(float min1, float max1, float min2, float max2, float v) {
  return min2 + (max2-min2) * (v-min1) / (max1-min1);
}

vec4 light_front = vec4(207, 227, 156, 255) / 255;
vec4 mid_front = vec4(205, 223, 131, 255) / 255;
vec4 dark = vec4(186, 206, 113, 240) / 255;
vec4 dark_back = vec4(186, 195, 108, 255) / 255;

vec4 front_col(float factor) {
  if (factor > 0.5) return light_front;
  else if (factor < 0.2) return dark;
  else if (factor < 0.4) {
    float f = (factor-0.2) / 0.2;
    return mix(dark, mid_front, f);
  } else {
    float f = 1 - (0.5-factor) / 0.1;
    return mix(mid_front, light_front, f);
  }
}

void main() {
  vec3 dir_to_light = vec3(0, 0, 1);
  float align_factor = dot(dir_to_light, gl_FrontFacing ? -normal : normal);

  FragColor = align_factor >= 0 ?
    front_col(align_factor) : mix(dark, dark_back, -align_factor);
}
