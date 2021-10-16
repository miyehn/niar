#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vf_normal;
layout(location = 1) in vec2 vf_uv;

layout(set = 0, binding = 1) uniform sampler2D BaseColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 light_dir = normalize(vec3(0, 0.5, 1));
    float brightness = clamp(dot(normalize(vf_normal), light_dir), 0, 1);
    // vec3 color = vec3(1);
    vec3 color = texture(BaseColor, vf_uv).rgb;

    outColor = vec4(color * mix(0.4f, 1.0f, brightness), 1);
    //outColor = vec4(vf_uv, 0, 1);
}