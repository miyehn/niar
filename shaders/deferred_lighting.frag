#version 460 core
#extension GL_EXT_ray_query : require

layout(location = 0) in vec2 vf_uv;
layout(location = 0) out vec4 FragColor;

#include "utils.glsl"
#include "scene_common.glsl" // (set 0, binding 0) view info
#include "lighting_common.glsl" // (set 0, bindings 5-9) pbr lighting functions

layout(set = 0, binding = 1) uniform sampler2D SceneDepth;
layout(set = 0, binding = 2) uniform sampler2D GBUF1;
layout(set = 0, binding = 3) uniform sampler2D GBUF2;
layout(set = 0, binding = 4) uniform sampler2D GBUF3;

#include "sky_common.glsl" // set 1, bindings 0-2; 8-9

void main() {

	FragColor = vec4(0, 0, 0, 1);

	ivec2 pixel = ivec2(gl_FragCoord.xy);
	float sceneDepth = texelFetch(SceneDepth, pixel, 0).r;
	vec4 GNormal = texelFetch(GBUF1, pixel, 0);
	vec4 GColor = texelFetch(GBUF2, pixel, 0);
	vec4 GORM = texelFetch(GBUF3, pixel, 0);

	ViewInfo viewInfo = GetViewInfo();

	float visibility = sceneDepth < 1.0f ? 1.0f : 0.0f;
	if (visibility > 0.5f) {
		vec3 worldPos = reconstructWorldPositionFromDepth(vf_uv, sceneDepth, viewInfo);
		// emission
		FragColor.rgb += vec3(GColor.a, GNormal.a, GORM.a);
		// the rest of lighting
		FragColor.rgb += accumulateLighting(
			worldPos,
			GNormal.xyz,
			GColor.rgb,
			GORM.rgb
		);
		vec3 indirectDiffuse = sampleIndirectLighting(vf_uv) * GColor.rgb;
		FragColor.rgb += indirectDiffuse;
	}
	else if (viewInfo.BackgroundOption > 0)
	{
		// environment map
		vec3 viewDirWS = screenSpaceUvToViewDir(vf_uv, viewInfo.ViewMatrix, viewInfo.HalfVFovRadians, viewInfo.AspectRatio);
		if (viewInfo.BackgroundOption == 1) {
			FragColor.rgb += sampleLongLatMap(EnvironmentMap, viewDirWS, 0);
		} else if (viewInfo.BackgroundOption == 2) {
			FragColor.rgb += sampleSkyAtmosphere(viewDirWS);
		}
	}
}
