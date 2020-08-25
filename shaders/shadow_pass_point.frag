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

layout(location=0) out float PositionLights[MaxLights];


void main() {

	for (int i=0; i<NumPointLights; i++) {

		vec3 LightToWorldPos = vf_position - PointLights[i].Position;
		float NearestDistToLight = texture(PointLights[i].ShadowMap, normalize(LightToWorldPos)).r;
		float DistToLight = length(LightToWorldPos);

		// slope-based bias
		float slope = abs(1.0f - dot(vf_normal, normalize(-LightToWorldPos)));
		float bias = mix(0.005, 0.01, slope);
		float Occlusion = DistToLight > NearestDistToLight+bias ? 0 : 1;

		PositionLights[i] = Occlusion;
	}

}
