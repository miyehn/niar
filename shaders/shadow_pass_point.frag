#version 330 core

const int MaxLights = 6;

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

uniform vec4 CameraParams;
uniform mat3 CAMERA_TO_WORLD_ROT;

uniform int NumPointLights;
uniform PointLight PointLights[MaxLights];

in vec3 vf_position;
in vec3 vf_normal;

layout(location=0) out float PositionLights[MaxLights];


void main() {

	float aspectRatio = CameraParams.z;
	float fov = CameraParams.w;
	vec2 ViewCoords = (gl_FragCoord.xy / CameraParams.xy) * 2.0f - 1.0f;
	ViewCoords.x *= aspectRatio;
	ViewCoords *= tan(fov / 2.0f);
	// ViewDir is working :)
	vec3 ViewDir = CAMERA_TO_WORLD_ROT * vec3(ViewCoords, -1);

	for (int i=0; i<NumPointLights; i++) {

		// TODO: should convert to eye depth
		float NearestDistToLight = texture(PointLights[i].ShadowMap, ViewDir).r;
		float DistToLight = length(PointLights[i].Position - vf_position);

		float bias = 0.005f;
		float Occlusion = DistToLight-NearestDistToLight >= bias ? 0.0f : 1.0f;

		// TODO: get rid of this temporary line
		//Occlusion = DistToLight > 4 ? 0 : 0.5;
		Occlusion = NearestDistToLight * 0.9f;

		PositionLights[i] = Occlusion;
	}

}
