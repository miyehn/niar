#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vf_normal;
layout(location = 1) in vec2 vf_uv;

// layout(location = 0) uniform sampler2D BaseColor;
// layout(location = 1) uniform vec3 Color;

layout(location = 0) out vec4 FragColor;

void main() {
    vec3 light_dir = normalize(vec3(0, 0.5, 1));
    float brightness = clamp(dot(normalize(vf_normal), light_dir), 0, 1);

    vec3 color = vec3(1, 1, 1);
//    vec3 color = texture(BaseColor, vf_uv).rgb * Color;

    FragColor = vec4(color * mix(0.4f, 1.0f, brightness), 1);
}
