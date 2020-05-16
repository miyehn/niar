#version 330 core

const int MaxShadowCastingLights = 2;

struct LightInfo {
	mat4 OBJECT_TO_CLIP;  // vertex
	sampler2D ShadowMap;
	vec3 Position;
	bool Directional;
	vec3 Direction;
};

uniform LightInfo LightInfos[MaxShadowCastingLights];

in vec3 vf_position;
in vec3 vf_normal;
in vec4 vf_PositionLights[MaxShadowCastingLights];

layout(location=0) out float PositionLights[MaxShadowCastingLights];


void main() {

	for (int i=0; i<MaxShadowCastingLights; i++) {
		vec3 LightSpacePosDivided = vf_PositionLights[i].xyz / vf_PositionLights[i].w;

		vec2 LightSpaceUV = (LightSpacePosDivided.xy + 1.0f) / 2.0f;

		float NearestDistToLight = texture(LightInfos[i].ShadowMap, LightSpaceUV).r;
		float DistToLight = (LightSpacePosDivided.z + 1.0f) / 2.0f;

		// offset
		vec3 DirToLight = LightInfos[i].Directional ? 
			-LightInfos[i].Direction : normalize(LightInfos[i].Position - vf_position);
		float bias = clamp(0.05 * (1.0f - dot(vf_normal, DirToLight)), 0, 0.005);

		float Occlusion = DistToLight-NearestDistToLight >= bias ? 0.0f : 1.0f;

		PositionLights[i] = Occlusion;
	}

}
