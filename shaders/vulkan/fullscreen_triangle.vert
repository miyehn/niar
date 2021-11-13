#version 450 core

vec2 positions[3] = vec2[](
  vec2(-1, -3),
  vec2(-1, 1),
  vec2(3, 1)
);

layout (location = 0) out vec2 vf_uv;

void main()
{
  gl_Position = vec4(positions[gl_VertexIndex], 0, 1);
  vf_uv = positions[gl_VertexIndex];
}