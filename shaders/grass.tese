#version 410 core

layout (quads) in;
uniform mat4 transformation;
in float c_gray[]; // length of TCS's output patch size

out float e_gray;

vec4 interpolate4v(in vec4 v0, in vec4 v1, in vec4 v2, in vec4 v3) {
  vec4 a = mix(v0, v1, gl_TessCoord.x);
  vec4 b = mix(v3, v2, gl_TessCoord.x);
  return mix(a, b, gl_TessCoord.y);
}

float interpolate1f(in float f0, in float f1, in float f2, in float f3) {
  float a = mix(f0, f1, gl_TessCoord.x);
  float b = mix(f3, f2, gl_TessCoord.x);
  return mix(a, b, gl_TessCoord.y);
}

void main() {
  gl_Position = transformation * interpolate4v(
    gl_in[0].gl_Position,
    gl_in[1].gl_Position,
    gl_in[2].gl_Position,
    gl_in[3].gl_Position
  );
  e_gray = (gl_TessCoord.x > 0.4 && gl_TessCoord.x < 0.6
      && gl_TessCoord.y > 0.4 && gl_TessCoord.y < 0.6) ? 1.0 : c_gray[0];
}
