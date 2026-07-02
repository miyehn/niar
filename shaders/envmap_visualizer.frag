#version 450 core

#include "scene_common.glsl"
#include "utils.glsl"

#include "sky_common.glsl"

layout (set = 0, binding = 7) uniform sampler2D EnvironmentMap;

layout(location=0) in vec4 vf_position;
layout(location=1) in vec2 vf_uv;
layout(location=2) in vec4 vf_currentClipPos;
layout(location=3) in vec4 vf_prevClipPos;
layout(location=4) in mat3 TANGENT_TO_WORLD_ROT;

layout(location=0) out vec4 outColor;

void main() {

    ViewInfo viewInfo = GetViewInfo();
    vec3 normal = TANGENT_TO_WORLD_ROT * vec3(0, 0, 1);
    vec3 sampleDir = reflectRay(normal, viewInfo.ViewDir);

    if (viewInfo.BackgroundOption == 0) {
        outColor = vec4(0.1, 0.1, 0.1, 1);
    } else if (viewInfo.BackgroundOption == 1){
        outColor = vec4(sampleLongLatMap(EnvironmentMap, sampleDir, 2), 1);
    } else if (viewInfo.BackgroundOption == 2) {
        outColor = vec4(sampleSkyAtmosphere(sampleDir), 1);
    }
}
