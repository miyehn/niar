#version 330 core

const int MaxLights = 6;

struct PointLight {
	vec3 Position;
	samplerCube ShadowMap;
};

in vec2 vf_uv;

uniform PointLight PointLights[MaxLights];
uniform sampler2D Position;
uniform sampler2D Normal;
uniform int NumPointLights;

#define NUM_OFFSETS 20
#define WEIGHT 0.04761904761
const vec3 offsets[NUM_OFFSETS] = vec3[] (
		vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
		vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
		vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
		vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
		vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
		);

layout(location=0) out float PositionLights[MaxLights];

void main() {

	vec3 position = texture(Position, vf_uv).xyz;
	vec3 normal = texture(Normal, vf_uv).xyz;

	for (int i=0; i<NumPointLights; i++) {

		vec3 LightToWorldPos = position - PointLights[i].Position;
		float NearestDistToLight = texture(PointLights[i].ShadowMap, normalize(LightToWorldPos)).r;
		float DistToLight = length(LightToWorldPos);

#if 1
		// slope-based bias
		float slope = 1.0f - clamp(dot(normal, normalize(-LightToWorldPos)), 0, 1);
		float bias = mix(0.03, 0.04, slope);
#else
		float bias = 0.03f;
#endif

		float Occlusion = float(DistToLight > NearestDistToLight + bias) * WEIGHT;
		for (int j=0; j<NUM_OFFSETS; j++) {
			LightToWorldPos = position - PointLights[i].Position + offsets[j]*0.03;
			NearestDistToLight = texture(PointLights[i].ShadowMap, LightToWorldPos).r;
			Occlusion += float(DistToLight > NearestDistToLight + bias) * WEIGHT;
		}

		float Lit2 = 1 - Occlusion;//sqrt(1 - Occlusion);
		PositionLights[i] = Lit2 > 0.5 ? 1 : mix(0, 1, Lit2*2);
	}

}
