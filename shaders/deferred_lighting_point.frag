#version 330 core

#define PI 3.14159265359

struct PointLight {
	vec3 position;
	bool castShadow;
	vec3 color;
	sampler2D shadowMask;
};

uniform sampler2D GBUF0; // world position
uniform sampler2D GBUF1; // normal
uniform sampler2D GBUF2; // base color

const int MaxLights = 6;

uniform int NumPointLights;
uniform PointLight PointLights[MaxLights];

uniform vec3 CameraPosition;

in vec2 vf_uv;
out vec4 FragColor;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1 - cosTheta, 5.0);
}

// gives the specular highlight
float distributionFn(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = dot(N, H);
	float NdotH2 = NdotH * NdotH;

	// denomenator
	float denom = NdotH2 * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;

	return a2 / denom;
}

float geometrySub(float NdotV, float roughness)
{
	float r = roughness + 1;
	float k = r * r / 8.0; // IBL needs something else here
	float denom = NdotV * (1.0 - k) + k;
	return NdotV / denom;
}

float geometrySmith(float NdotL, float NdotV, float roughness)
{
	float g1 = geometrySub(NdotV, roughness);
	float g2 = geometrySub(NdotL, roughness);
	return g1 * g2;
}

void main() {

	vec4 GPosition = texture(GBUF0, vf_uv);
	vec4 GNormal = texture(GBUF1, vf_uv);
	vec4 GColor = texture(GBUF2, vf_uv);

	vec3 position = GPosition.xyz;
	vec3 normal = GNormal.xyz;
	vec3 albedo = GColor.rgb;
	// TODO
	float metallic = 0;
	float roughness = 0.3;
	float ambientOcclusion = 0.03;

	// other light-independent properties
	vec3 viewDir = normalize(position - CameraPosition);
	float NdotV = max(dot(normal, -viewDir), 0);

	FragColor = vec4(0, 0, 0, 1);
	for (int i=0; i<NumPointLights; i++)
	{
		// some useful properties
		vec3 lightDir = position - PointLights[i].position;
		float atten = 1.0 / dot(lightDir, lightDir);
		lightDir = normalize(lightDir);
		vec3 halfVec = -normalize(viewDir + lightDir); // TODO: just use normal here??
		float NdotL = max(dot(normal, -lightDir), 0);
		vec3 radiance = PointLights[i].color * atten;

		//---- specular ----

		// Fresnel
		vec3 F0 = vec3(0.04); // base reflectivity for non-metals
		F0 = mix(F0, albedo, metallic); // if metal, use what's in albedo map for base reflectivity
		vec3 F = fresnelSchlick( max(dot(halfVec, -viewDir), 0), F0 ); // TODO: use normal or H here?

		// Distribution
		float D = distributionFn(normal, halfVec, roughness);

		// Geometry
		float G = geometrySmith(NdotL, NdotV, roughness);

		// specular
		vec3 num = F * D * G;
		float denom = 4.0 * NdotV * NdotL;
		vec3 specular = num / max(denom, 0.001);

		//---- diffuse ----

		vec3 kSpecular = F;
		vec3 kDiffuse = vec3(1.0) - kSpecular;
		kDiffuse *= 1.0 - metallic;
		vec3 diffuse = kDiffuse * albedo / PI;

		//---- contribution ----
		vec3 Lo = (diffuse + specular) * radiance * NdotL;

		if (PointLights[i].castShadow) {
			float Occlusion = texture(PointLights[i].shadowMask, vf_uv).r;
			Lo *= Occlusion;
		}
		FragColor.rgb += Lo;
	}

}
