#version 450 core

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 ModelMatrix;
  mat4 ViewMatrix;
  mat4 ProjectionMatrix;
  vec3 Tint;
} ubo;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_tangent;
layout (location = 3) in vec2 in_uv;

layout (location = 0) out vec4 vf_position;
layout (location = 1) out vec2 vf_uv;
layout (location = 2) out mat3 TANGENT_TO_WORLD_ROT;

void main() {
  gl_Position = ubo.ProjectionMatrix * ubo.ViewMatrix * ubo.ModelMatrix * vec4(in_position, 1.0);//OBJECT_TO_CLIP * vec4(in_position, 1);
  vf_position = ubo.ModelMatrix * vec4(in_position, 1.0);

  vf_uv = in_uv;

  mat3 OBJECT_TO_WORLD_ROT = mat3(ubo.ModelMatrix);

  vec3 N = OBJECT_TO_WORLD_ROT * in_normal;
  vec3 T = OBJECT_TO_WORLD_ROT * in_tangent;
  vec3 B = cross(N, T);
  TANGENT_TO_WORLD_ROT = mat3(T, B, N);
}
