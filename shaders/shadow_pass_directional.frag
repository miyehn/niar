#version 330 core

const int MaxLights = 6;

struct DirectionalLight {
	mat4 OBJECT_TO_CLIP; // vertex
	sampler2D ShadowMap;
	vec3 Direction;
};

uniform int NumDirectionalLights;
uniform DirectionalLight DirectionalLights[MaxLights];

in vec3 vf_position;
in vec3 vf_normal;
in vec4 vf_DirectionalLightSpacePositions[MaxLights];

layout(location=0) out float PositionLights[MaxLights];


void main() {

	for (int i=0; i<NumDirectionalLights; i++) {
		vec3 LightSpacePos = vf_DirectionalLightSpacePositions[i].xyz;
		vec2 LightSpaceUV = (LightSpacePos.xy + 1.0f) / 2.0f;

		// both linear because it's orthographic projection
		float NearestDistToLight = texture(DirectionalLights[i].ShadowMap, LightSpaceUV).r;
		float DistToLight = (LightSpacePos.z + 1.0f) / 2.0f;

		// some corrections for acne and peter-panning...
		vec3 DirToLight = -DirectionalLights[i].Direction;
		float bias = clamp(0.005 * (1.0f - dot(vf_normal, DirToLight)), 0, 0.005);
		float Occlusion = DistToLight-NearestDistToLight >= bias ? 0.0f : 1.0f;
		if (DistToLight > 1) Occlusion = 1.0f;

		PositionLights[i] = Occlusion;
	}
}
