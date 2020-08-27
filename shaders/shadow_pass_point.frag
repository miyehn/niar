#version 330 core

const int MaxLights = 6;

struct PointLight {
	vec3 Position;
	samplerCube ShadowMap;
};

uniform int NumPointLights;
uniform PointLight PointLights[MaxLights];

in vec3 vf_position;
in vec3 vf_normal;

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

	for (int i=0; i<NumPointLights; i++) {

		vec3 LightToWorldPos = vf_position - PointLights[i].Position;
		float NearestDistToLight = texture(PointLights[i].ShadowMap, normalize(LightToWorldPos)).r;
		float DistToLight = length(LightToWorldPos);

		// slope-based bias
		//float slope = 1.0f - clamp(dot(vf_normal, normalize(-LightToWorldPos)), 0, 1);
		float bias = 0.05;//mix(0.03, 0.08, slope);

		float Occlusion = float(DistToLight > NearestDistToLight + bias) * WEIGHT;
		for (int j=0; j<NUM_OFFSETS; j++) {
			LightToWorldPos = vf_position - PointLights[i].Position + offsets[j]*0.02;
			NearestDistToLight = texture(PointLights[i].ShadowMap, LightToWorldPos).r;
			Occlusion += float(DistToLight > NearestDistToLight + bias) * WEIGHT;
		}

		PositionLights[i] = 1.0f - Occlusion;
	}

}
