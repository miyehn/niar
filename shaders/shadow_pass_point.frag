#version 330 core

const int MaxLights = 6;

struct PointLight {
	vec3 Position;
	samplerCube ShadowMap;
};

uniform vec4 CameraParams;
uniform mat3 CAMERA_TO_WORLD_ROT;

uniform int NumPointLights;
uniform PointLight PointLights[MaxLights];

in vec3 vf_position;
in vec3 vf_normal;

layout(location=0) out float PositionLights[MaxLights];


void main() {

	/*
	float aspectRatio = CameraParams.z;
	float fov = CameraParams.w;
	vec2 ViewCoords = (gl_FragCoord.xy / CameraParams.xy) * 2.0f - 1.0f;
	ViewCoords.x *= aspectRatio;
	ViewCoords *= tan(fov / 2.0f);
	// ViewDir is working :)
	vec3 ViewDir = CAMERA_TO_WORLD_ROT * vec3(ViewCoords, -1);
	*/

	for (int i=0; i<NumPointLights; i++) {

		vec3 LightToWorldPos = vf_position - PointLights[i].Position;
		float NearestDistToLight = texture(PointLights[i].ShadowMap, normalize(LightToWorldPos)).r;
		float DistToLight = length(LightToWorldPos);

		float bias = 0.005;
		float Occlusion = DistToLight > NearestDistToLight-bias ? 0 : 1;

		// TODO: get rid of these temporary lines
		//Occlusion = DistToLight > 4 ? 0.7 : 0.9;
		//Occlusion = clamp(NearestDistToLight * 0.2, 0, 1) * 0.9f;

		PositionLights[i] = Occlusion;
	}

}
