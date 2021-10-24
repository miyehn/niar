#version 450 core

#include "rendertargets.glsl"
#include "scene_common.glsl"

#define PI 3.14159265359

struct PointLight {
	vec3 position;
	vec3 color;
};

const int MaxLights = 4;
layout(set = 3, binding = 0) uniform LightsInfo
{
	PointLight[MaxLights] Lights;
} ubo;

layout(location = 0) out vec4 FragColor;

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

	vec4 GPosition = subpassLoad(GBUF0);
	vec4 GNormal = subpassLoad(GBUF1);
	vec4 GColor = subpassLoad(GBUF2);
	vec4 GMRA = subpassLoad(GBUF3);

	vec3 position = GPosition.xyz;
	vec3 normal = GNormal.xyz;
	vec3 albedo = GColor.rgb;
	float metallic = GMRA.r;
	float roughness = GMRA.g;
	float ambientOcclusion = GMRA.b;

	ViewInfo viewInfo = GetViewInfo();

	int NumLights = viewInfo.NumPointLights;

	// other light-independent properties
	vec3 viewDir = viewInfo.ViewDir;
	float NdotV = max(dot(normal, -viewDir), 0);

	FragColor = vec4(0, 0, 0, 1);
	for (int i = 0; i < NumLights; i++)
	{
		// some useful properties
		vec3 lightDir = position - ubo.Lights[i].position;
		float atten = 1.0 / dot(lightDir, lightDir);
		lightDir = normalize(lightDir);
		vec3 halfVec = -normalize(viewDir + lightDir); // TODO: just use normal here??
		float NdotL = max(dot(normal, -lightDir), 0);
		vec3 radiance = ubo.Lights[i].color * atten;

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
		vec3 Lo = (diffuse + specular) * radiance * NdotL * (1 - ambientOcclusion);

		FragColor.rgb += Lo;
	}

}
