#version 450 core

#include "scene_common.glsl"

layout(push_constant) uniform GltfVertexPushConstants {
  mat4 ModelMatrix;
} pc;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec4 in_tangent;
layout (location = 3) in vec2 in_uv;

layout (location = 0) out vec4 vf_position;
layout (location = 1) out vec2 vf_uv;
layout (location = 2) out vec4 vf_currentClipPos;
layout (location = 3) out vec4 vf_prevClipPos;
layout (location = 4) out mat3 TANGENT_TO_WORLD_ROT;

void main()
{
  ViewInfo viewInfo = GetViewInfo();

  vec4 worldPos4 = pc.ModelMatrix * vec4(in_position, 1.0);
  gl_Position = viewInfo.ProjectionMatrix * viewInfo.ViewMatrix * worldPos4;
  vf_position = worldPos4 - vec4(viewInfo.CameraPosition, 0);

  vf_uv = in_uv;

  vf_currentClipPos = gl_Position;
  vf_prevClipPos = viewInfo.PrevProjectionMatrix * viewInfo.PrevViewMatrix * worldPos4;

  // subpixel jitter for TAA, applied after capturing the unjittered clip positions above so
  // motion vectors (derived from vf_currentClipPos/vf_prevClipPos in geometry.frag) stay jitter-free.
  // Remember to check against RenderSize being unset (0,0): dividing by it would send every vertex to
  // infinite clip-space and make nothing rasterize.
  vec2 jitterClipSpace = (viewInfo.JitterOffset / viewInfo.RenderSize) * 2.0f;
  // gl_Position.xy is to be divided by w to become NDC, so multiply by w here to cancel out
  gl_Position.xy += jitterClipSpace * gl_Position.w;

  mat3 OBJECT_TO_WORLD_ROT = mat3(pc.ModelMatrix);

  vec3 N = normalize(OBJECT_TO_WORLD_ROT * in_normal);
  vec3 T = normalize(OBJECT_TO_WORLD_ROT * in_tangent.xyz);
  vec3 B = cross(N, T) * in_tangent.w;
  TANGENT_TO_WORLD_ROT = mat3(T, B, N);
}
