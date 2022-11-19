#version 450 core

#include "utils.glsl"
#include "scene_common.glsl" // (set 0, binding 0; 5-7) frameglobal
#include "lighting_common.glsl" // pbr lighting functions
#include "rendertargets.glsl" // (set 0, bindings 1-4) g buffers

#include "sky_common.glsl" // set 1, bindings 0-2; 8-9

layout (set = 0, binding = 7) uniform sampler2D EnvironmentMap;

layout(location = 0) in vec2 vf_uv;

layout(location = 0) out vec4 FragColor;

void main() {

	FragColor = vec4(0, 0, 0, 1);

	vec4 GPosition = subpassLoad(GBUF0);
	vec4 GNormal = subpassLoad(GBUF1);
	vec4 GColor = subpassLoad(GBUF2);
	vec4 GORM = subpassLoad(GBUF3);

	ViewInfo viewInfo = GetViewInfo();

	float visibility = GColor.a;
	if (visibility > 0.5f) {
		// emission
		FragColor.rgb += vec3(GPosition.a, GNormal.a, GORM.a);
		// the rest of lighting
		FragColor.rgb += accumulateLighting(
			GPosition.xyz + viewInfo.CameraPosition,
			GNormal.xyz,
			GColor.rgb,
			GORM.rgb
		);
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
