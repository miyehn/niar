#version 450 core

#include "scene_common.glsl"

layout(set = 3, binding = 0) uniform sampler2D SceneColor;
layout(set = 3, binding = 1) uniform sampler2D SceneDepth;

layout(location = 0) in vec2 vf_uv;
layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = texture(SceneColor, vf_uv) * vec4(1, 0.5, 0.2, 1);
}