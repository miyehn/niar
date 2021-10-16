#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 ModelMatrix;
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec3 vf_normal;
layout(location = 1) out vec2 vf_uv;

void main() {
    gl_Position = ubo.ProjectionMatrix * ubo.ViewMatrix * ubo.ModelMatrix * vec4(inPosition, 1.0);
    vf_normal = mat3(ubo.ModelMatrix) * inNormal;
    vf_uv = in_uv;
}