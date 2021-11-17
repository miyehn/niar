#version 450 core

#include "scene_common.glsl"

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec4 in_color;

layout (location = 0) out vec4 vf_color;

void main()
{
    ViewInfo viewInfo = GetViewInfo();

    gl_Position = viewInfo.ProjectionMatrix * viewInfo.ViewMatrix * vec4(in_position.xyz, 1.0);
    gl_PointSize = 1 + 20 * (1.0f / gl_Position.z);
    vf_color = in_color;
}