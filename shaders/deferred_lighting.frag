#version 450 core

#include "rendertargets.glsl"
#include "scene_common.glsl"
#include "utils.glsl"

struct PointLight {
	vec3 position;
	vec3 color;
};

struct DirectionalLight {
	vec3 direction;
	vec3 color;
};

const int MaxLights = 128; // 4KB if each light takes { vec4, vec4 }. must match c++

layout (set = 3, binding = 0) uniform PointLightsInfo {
	PointLight[MaxLights] Data;
} PointLights;

layout (set = 3, binding = 1) uniform DirectionalLightsInfo {
	DirectionalLight[MaxLights] Data;
} DirectionalLights;

layout (set = 3, binding = 2) uniform sampler2D EnvironmentMap;

layout(location = 0) in vec2 vf_uv;

layout(location = 0) out vec4 FragColor;

vec3 fresnelSchlick(float VdotH, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1 - VdotH, 5.0);
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

	ViewInfo viewInfo = GetViewInfo();

	vec4 GPosition = subpassLoad(GBUF0);
	vec4 GNormal = subpassLoad(GBUF1);
	vec4 GColor = subpassLoad(GBUF2);
	vec4 GMRAV = subpassLoad(GBUF3);

	vec3 position = GPosition.xyz + viewInfo.CameraPosition;
	vec3 normal = GNormal.xyz;
	vec3 albedo = GColor.rgb;
	float metallic = GMRAV.r;
	float roughness = GMRAV.g;
	// float ambientOcclusion = GMRAV.b;
	float visibility = GMRAV.a;

	// other light-independent properties
	vec3 dirToCam = normalize(viewInfo.CameraPosition - position);
	float NdotV = max(0, dot(normal, dirToCam));

	FragColor = vec4(0, 0, 0, 1);

	bool hit = visibility > 0;
	if (hit)
	{
		// point lights
		for (int i = 0; i < viewInfo.NumPointLights; i++)
		{
			// some useful properties
			vec3 dirToLight = PointLights.Data[i].position - position;
			float atten = 1.0 / dot(dirToLight, dirToLight);
			dirToLight = normalize(dirToLight);
			vec3 halfVec = normalize(dirToCam + dirToLight); // TODO: just use normal here??
			float NdotL = max(dot(normal, dirToLight), 0);
			vec3 radiance = PointLights.Data[i].color * atten;

			//---- specular ----

			// Fresnel
			vec3 F0 = vec3(0.04); // base reflectivity for non-metals
			F0 = mix(F0, albedo, metallic); // if metal, use what's in albedo map for base reflectivity
			vec3 F = fresnelSchlick( max(dot(halfVec, dirToCam), 0), F0 );

			// Distribution
			float D = distributionFn(normal, halfVec, roughness);

			// Geometry
			float G = geometrySmith(NdotL, NdotV, roughness);

			// specular (cook tolerance)
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

			FragColor.rgb += Lo;
		}

		// directional lights
		for (int i = 0; i < viewInfo.NumDirectionalLights; i++)
		{
			vec3 lightDir = DirectionalLights.Data[i].direction;
			vec3 halfVec = normalize(dirToCam - lightDir);
			float NdotL = max(dot(normal, -lightDir), 0);
			vec3 radiance = DirectionalLights.Data[i].color;

			// Fresnel
			vec3 F0 = vec3(0.04); // base reflectivity for non-metals
			F0 = mix(F0, albedo, metallic); // if metal, use what's in albedo map for base reflectivity
			vec3 F = fresnelSchlick( max(dot(halfVec, dirToCam), 0), F0 ); // TODO: use normal or H here?

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

			FragColor.rgb += Lo;
		}
	}
	else if (viewInfo.UseEnvironmentMap > 0)
	{
		// environment map
		vec3 viewDirWS = screenSpaceUvToViewDir(
			vf_uv, viewInfo.ViewMatrix, viewInfo.HalfVFovRadians, viewInfo.AspectRatio);
		FragColor.rgb += sampleLongLatMap(EnvironmentMap, viewDirWS, 0);
	}
}
