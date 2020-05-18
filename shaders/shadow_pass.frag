#version 330 core

const int MaxShadowCastingLights = 2;

struct LightInfo {
	mat4 OBJECT_TO_CLIP;  // vertex
	sampler2D ShadowMap;
	vec3 Position;
	bool Directional;
	vec3 Direction;
};

struct DirectionalLight {
	mat4 OBJECT_TO_CLIP; // vertex
	sampler2D ShadowMap;
	vec3 Direction;
};

struct PointLight {
	vec3 Position;
	sampler2D ShadowMap; // TODO!!!!!
};

// todo: remove
uniform LightInfo LightInfos[MaxShadowCastingLights];

uniform int NumDirectionalLights;
uniform int NumPointLights;

uniform DirectionalLight DirectionalLights[MaxShadowCastingLights];
uniform PointLight PointLights[MaxShadowCastingLights];

in vec3 vf_position;
in vec3 vf_normal;
in vec4 vf_DirectionalLightSpacePositions[MaxShadowCastingLights];

layout(location=0) out float PositionLights[MaxShadowCastingLights];


void main() {

	for (int i=0; i<NumPointLights; i++) {
		vec3 ViewDir = vec3(1);

		// TODO!!
		float NearestDistToLight = 0.0f;//texture(PointLights[i].ShadowMap, ViewDir).r;
		float DistToLight = length(PointLights[i].Position - vf_position);

		float bias = 0.005f;
		float Occlusion = DistToLight-NearestDistToLight >= bias ? 0.0f : 1.0f;
		if (DistToLight > 1) Occlusion = 1.0f;

		PositionLights[i] = Occlusion;
	}


	for (int i=0; i<MaxShadowCastingLights; i++) {
		vec3 LightSpacePosDivided = vf_DirectionalLightSpacePositions[i].xyz / vf_DirectionalLightSpacePositions[i].w;

		vec2 LightSpaceUV = (LightSpacePosDivided.xy + 1.0f) / 2.0f;

		float NearestDistToLight = texture(LightInfos[i].ShadowMap, LightSpaceUV).r;
		float DistToLight = (LightSpacePosDivided.z + 1.0f) / 2.0f;

		// offset
		vec3 DirToLight = LightInfos[i].Directional ? 
			-LightInfos[i].Direction : normalize(LightInfos[i].Position - vf_position);
		float bias = clamp(0.005 * (1.0f - dot(vf_normal, DirToLight)), 0, 0.005);

		float Occlusion = DistToLight-NearestDistToLight >= bias ? 0.0f : 1.0f;
		if (DistToLight > 1) Occlusion = 1.0f;

		PositionLights[i] = Occlusion;
	}

	// TODO: why wrong?
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

}
