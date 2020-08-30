#version 330 core

const int MaxLights = 6;

struct DirectionalLight {
	mat4 WORLD_TO_LIGHT_CLIP;
	sampler2D ShadowMap;
	vec3 Direction;
};

in vec2 vf_uv;

uniform DirectionalLight DirectionalLights[MaxLights];
uniform sampler2D Position;
uniform sampler2D Normal;
uniform int NumDirectionalLights;

layout(location=0) out float PositionLights[MaxLights];

#define NUM_OFFSETS 4
#define WEIGHT 0.2
const vec2 offsets[NUM_OFFSETS] = vec2[] (
		vec2(1, 1), vec2(-1, -1),
		vec2(1, -1), vec2(-1, 1)
		);

void main() {

	vec3 normal = texture(Normal, vf_uv).xyz;

	for (int i=0; i<NumDirectionalLights; i++) {
		vec4 LightSpacePos4 = DirectionalLights[i].WORLD_TO_LIGHT_CLIP * vec4(texture(Position, vf_uv).xyz, 1);
		vec3 LightSpacePos = LightSpacePos4.xyz / LightSpacePos4.w;
		vec2 LightSpaceUV = (LightSpacePos.xy + 1.0f) / 2.0f;

		// both linear because it's orthographic projection
		float NearestDistToLight = texture(DirectionalLights[i].ShadowMap, LightSpaceUV).r;
		float DistToLight = (LightSpacePos.z + 1.0f) / 2.0f;

		// slope-based bias
		float slope = 1.0f - clamp(dot(normal, -DirectionalLights[i].Direction), 0, 1);
		float bias = mix(0.001, 0.01, slope);

		float Occlusion = float(DistToLight-NearestDistToLight >= bias) * WEIGHT;
		for (int j=0; j<NUM_OFFSETS; j++) {
			NearestDistToLight = texture(DirectionalLights[i].ShadowMap, LightSpaceUV + offsets[j]*0.0008).r;
			Occlusion += float(DistToLight-NearestDistToLight >= bias) * WEIGHT;
		}

		PositionLights[i] = 1.0f - Occlusion;
	}
}
