#version 450 core

#include "utils.glsl"

layout(set = 3, binding = 1) uniform sampler2D EnvironmentMap;

layout(location=0) in vec4 vf_position;
layout(location=1) in vec2 vf_uv;
layout(location=2) in mat3 TANGENT_TO_WORLD_ROT;

layout(location=0) out vec4 outColor;

void main() {
    vec3 normal = TANGENT_TO_WORLD_ROT * vec3(0, 0, 1);
    outColor = vec4(sampleLongLatMap(EnvironmentMap, normal, 2), 1);
}
