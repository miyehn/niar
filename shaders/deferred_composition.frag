#version 330 core

struct DirectionalLight {
	vec3 direction;
	vec3 color;
};

struct PointLight {
	vec3 position;
	vec3 color;
};

uniform sampler2D GBUF0;
uniform sampler2D GBUF1;
uniform sampler2D GBUF2;

const int MaxDirectionalLights = 4;
const int MaxPointLights = 4;

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
		FragColor.rgb += NdotL * DirectionalLights[i].color * color;
	}

	for (int i=0; i<NumPointLights; i++) {
		vec3 lightDir = position - PointLights[i].position;
		float atten = 1.0f / dot(lightDir, lightDir);
		lightDir = normalize(lightDir);
		float NdotL = dot(normal, -lightDir);
		FragColor.rgb += NdotL * PointLights[i].color * color * atten;
	}

}

