#version 330 core

struct DirectionalLight {
	vec3 direction;
	bool castShadow;
	vec3 color;
	sampler2D shadowMask;
};

struct PointLight {
	vec3 position;
	bool castShadow;
	vec3 color;
	sampler2D shadowMask;
};

uniform sampler2D GBUF0; // world position
uniform sampler2D GBUF1; // normal
uniform sampler2D GBUF2; // base color

const int MaxDirectionalLights = 2;
const int MaxPointLights = 2;

uniform int NumDirectionalLights;
uniform int NumPointLights;

uniform DirectionalLight DirectionalLights[MaxDirectionalLights];
uniform PointLight PointLights[MaxPointLights];

in vec2 vf_uv;
out vec4 FragColor;

void main() {

	vec4 GPosition = texture(GBUF0, vf_uv);
	vec4 GNormal = texture(GBUF1, vf_uv);
	vec4 GColor = texture(GBUF2, vf_uv);

	vec3 position = GPosition.xyz;
	vec3 normal = GNormal.xyz;
	vec3 color = GColor.rgb;

	FragColor = vec4(0, 0, 0, 1);
	
	for (int i=0; i<NumDirectionalLights; i++) {
		float NdotL = dot(normal, -DirectionalLights[i].direction);
		vec3 Contrib = max(vec3(0), NdotL * DirectionalLights[i].color * color);
		if (DirectionalLights[i].castShadow) {
			float Occlusion = texture(DirectionalLights[i].shadowMask, vf_uv).r;
			Contrib *= Occlusion;
		}
		FragColor.rgb += Contrib;
	}

	for (int i=0; i<NumPointLights; i++) {
		vec3 lightDir = position - PointLights[i].position;
		float atten = 1.0f / dot(lightDir, lightDir);
		lightDir = normalize(lightDir);
		float NdotL = dot(normal, -lightDir);
		FragColor.rgb += max(vec3(0), NdotL * PointLights[i].color * color * atten);
	}

}
