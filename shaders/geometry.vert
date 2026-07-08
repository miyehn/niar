#version 450 core

#include "scene_common.glsl"

layout(push_constant) uniform GltfVertexPushConstants {
  mat4 ModelMatrix;
} pc;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec4 in_tangent;
layout (location = 3) in vec2 in_uv;

layout (location = 0) out vec4 vf_relWorldPos;
layout (location = 1) out vec2 vf_uv;
layout (location = 2) out vec4 vf_currentClipPos;
layout (location = 3) out vec4 vf_prevClipPos;
layout (location = 4) out mat3 TANGENT_TO_WORLD_ROT;

void main()
{
  ViewInfo viewInfo = GetViewInfo();

  vec4 worldPos4 = pc.ModelMatrix * vec4(in_position, 1.0);
  // ProjectionMatrix already carries the TAA jitter baked in on the CPU side (see
  // Renderer::getCameraViewInfo), so rasterization here and screen-space reconstruction in the
  // lighting pass both see the same jittered camera.
  gl_Position = viewInfo.ProjectionMatrix * viewInfo.ViewMatrix * worldPos4;
  vf_relWorldPos = worldPos4 - vec4(viewInfo.CameraPosition, 0);

  vf_uv = in_uv;

  // motion vectors must stay jitter-free, so use the unjittered projection here instead of gl_Position
  vf_currentClipPos = viewInfo.UnjitteredProjectionMatrix * viewInfo.ViewMatrix * worldPos4;
  vf_prevClipPos = viewInfo.PrevUnjitteredProjectionMatrix * viewInfo.PrevViewMatrix * worldPos4;

  mat3 OBJECT_TO_WORLD_ROT = mat3(pc.ModelMatrix);

  vec3 N = normalize(OBJECT_TO_WORLD_ROT * in_normal);
  vec3 T = normalize(OBJECT_TO_WORLD_ROT * in_tangent.xyz);
  vec3 B = cross(N, T) * in_tangent.w;
  TANGENT_TO_WORLD_ROT = mat3(T, B, N);
}
