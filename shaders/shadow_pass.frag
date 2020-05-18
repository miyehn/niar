#version 330 core

const int MaxLights = 6;

struct DirectionalLight {
	mat4 OBJECT_TO_CLIP; // vertex
	sampler2D ShadowMap;
	vec3 Direction;
};

struct PointLight {
	vec3 Position;
	samplerCube ShadowMap;
};

/* Ensure total number of lights <= MaxLights (now 3)
 *
 * Layout:
 * 0 - 2D (directional)
 * 1 - 2D (directional)
 * 2 - 2D (dummy)
 * 3 - 2D (dummy)
 * ...
 * 6 - Cube (point)
 * 7 - Cube (dummy)
 * 8 - Cube (dummy)
 * ...
 */

uniform int NumDirectionalLights;
uniform int NumPointLights;

uniform DirectionalLight DirectionalLights[MaxLights];
uniform PointLight PointLights[MaxLights];

in vec3 vf_position;
in vec3 vf_normal;
in vec4 vf_DirectionalLightSpacePositions[MaxLights];

layout(location=0) out float PositionLights[MaxLights];


void main() {

	for (int i=0; i<NumDirectionalLights; i++) {
		vec3 LightSpacePosDivided = vf_DirectionalLightSpacePositions[i].xyz / vf_DirectionalLightSpacePositions[i].w;
		vec2 LightSpaceUV = (LightSpacePosDivided.xy + 1.0f) / 2.0f;

		float NearestDistToLight = texture(DirectionalLights[i].ShadowMap, LightSpaceUV).r;
		float DistToLight = (LightSpacePosDivided.z + 1.0f) / 2.0f;

		vec3 DirToLight = -DirectionalLights[i].Direction;
		float bias = clamp(0.005 * (1.0f - dot(vf_normal, DirToLight)), 0, 0.005);
		float Occlusion = DistToLight-NearestDistToLight >= bias ? 0.0f : 1.0f;
		if (DistToLight > 1) Occlusion = 1.0f;

		PositionLights[i] = Occlusion;
	}

	for (int i=0; i<NumPointLights; i++) {
		// TODO!!
		vec3 ViewDir = vec3(1);

		float NearestDistToLight = texture(PointLights[i].ShadowMap, ViewDir).r;
		float DistToLight = length(PointLights[i].Position - vf_position);

		float bias = 0.005f;
		float Occlusion = DistToLight-NearestDistToLight >= bias ? 0.0f : 1.0f;
		if (DistToLight > 1) Occlusion = 1.0f;

		PositionLights[NumDirectionalLights + i] = 0.0f;//Occlusion;
	}

}
