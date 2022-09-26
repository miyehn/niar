struct PointLight {
    vec3 position;
    vec3 color;
};

struct DirectionalLight {
    vec3 direction;
    vec3 color;
};

const int MaxLights = 128; // 4KB if each light takes { vec4, vec4 }. must match c++

layout (set = 0, binding = 5) uniform PointLightsInfo {
    PointLight[MaxLights] Data;
} PointLights;

layout (set = 0, binding = 6) uniform DirectionalLightsInfo {
    DirectionalLight[MaxLights] Data;
} DirectionalLights;

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

struct MaterialLightingInfo {
// common
    vec3 albedo;
    float metallic;
    vec3 normal;
    float roughness;
    vec3 dirToCam;
    float NdotV;
// light-specific
    vec3 halfVec;
    float NdotL;
};

vec3 lightingContrib(MaterialLightingInfo info)
{
    //---- specular ----

    // Fresnel
    vec3 F0 = vec3(0.04); // base reflectivity for non-metals
    F0 = mix(F0, info.albedo, info.metallic); // if metal, use what's in albedo map for base reflectivity
    vec3 F = fresnelSchlick( max(dot(info.halfVec, info.dirToCam), 0), F0 );

    // Distribution
    float D = distributionFn(info.normal, info.halfVec, info.roughness);

    // Geometry
    float G = geometrySmith(info.NdotL, info.NdotV, info.roughness);

    // specular (cook tolerance)
    vec3 num = F * D * G;
    float denom = 4.0 * info.NdotV * info.NdotL;
    vec3 specular = num / max(denom, 0.001);

    //---- diffuse ----

    vec3 kSpecular = F;
    vec3 kDiffuse = vec3(1.0) - kSpecular;
    kDiffuse *= 1.0 - info.metallic;
    vec3 diffuse = kDiffuse * info.albedo / PI;

    //---- contribution ----
    return (diffuse + specular) * info.NdotL;
}

// assumes frameglobal stuff is available and fragment is visible
vec3 accumulateLighting(vec3 worldPos, vec3 normal, vec3 albedo, vec3 orm)
{
    ViewInfo viewInfo = GetViewInfo();

    MaterialLightingInfo info;
    info.albedo = albedo;
    info.metallic = orm.b;
    info.normal = normal;
    info.roughness = orm.g;
    info.dirToCam = normalize(viewInfo.CameraPosition - worldPos);
    info.NdotV = max(0, dot(normal, info.dirToCam));

    vec3 result = vec3(0, 0, 0);
    // point lights
    for (int i = 0; i < viewInfo.NumPointLights; i++)
    {
        // some useful properties
        vec3 dirToLight = PointLights.Data[i].position - worldPos;
        float atten = 1.0 / dot(dirToLight, dirToLight);
        dirToLight = normalize(dirToLight);
        vec3 halfVec = normalize(info.dirToCam + dirToLight);
        float NdotL = max(dot(normal, dirToLight), 0);
        vec3 radiance = PointLights.Data[i].color * atten;

        info.halfVec = halfVec;
        info.NdotL = NdotL;
        result += lightingContrib(info) * radiance;
    }

    // directional lights
    for (int i = 0; i < viewInfo.NumDirectionalLights; i++)
    {
        vec3 lightDir = DirectionalLights.Data[i].direction;
        vec3 halfVec = normalize(info.dirToCam - lightDir);
        float NdotL = max(dot(normal, -lightDir), 0);
        vec3 radiance = DirectionalLights.Data[i].color;

        info.halfVec = halfVec;
        info.NdotL = NdotL;
        result += lightingContrib(info) * radiance;
    }

    return result;
}

