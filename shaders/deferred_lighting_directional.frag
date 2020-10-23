#version 330 core

#define PI 3.14159265359

struct DirectionalLight {
	vec3 direction;
	bool castShadow;
	vec3 color;
	sampler2D shadowMask;
};

uniform sampler2D GBUF0; // world position
uniform sampler2D GBUF1; // normal
uniform sampler2D GBUF2; // base color
uniform sampler2D GBUF3; // metallic, roughness, AO

const int MaxLights = 6;

uniform int NumDirectionalLights;
uniform DirectionalLight DirectionalLights[MaxLights];

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
	vec4 GMRA = texture(GBUF3, vf_uv);

	vec3 position = GPosition.xyz;
	vec3 normal = GNormal.xyz;
	vec3 albedo = GColor.rgb;
	float metallic = GMRA.r;
	float roughness = GMRA.g;
	float ambientOcclusion = GMRA.b;

	// other light-independent properties
	vec3 viewDir = normalize(position - CameraPosition);
	float NdotV = max(dot(normal, -viewDir), 0);

	FragColor = vec4(0, 0, 0, 1);
	for (int i=0; i<NumDirectionalLights; i++)
	{
		vec3 lightDir = DirectionalLights[i].direction;
		vec3 halfVec = -normalize(viewDir + lightDir);
		float NdotL = max(dot(normal, -lightDir), 0);
		vec3 radiance = DirectionalLights[i].color;

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
		if (DirectionalLights[i].castShadow) {
			float Occlusion = texture(DirectionalLights[i].shadowMask, vf_uv).r;
			Lo *= Occlusion;
		}
		FragColor.rgb += Lo;
	}

}
