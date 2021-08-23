#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 ModelMatrix;
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;

void main() {
    // gl_Position = vec4(inPosition, 1.0);
    gl_Position = ubo.ProjectionMatrix * ubo.ViewMatrix * ubo.ModelMatrix * vec4(inPosition, 1.0);
    fragColor = inNormal;
}