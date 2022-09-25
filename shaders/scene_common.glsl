struct ViewInfo
{
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;

    vec3 CameraPosition;
    int NumPointLights;

    vec3 ViewDir;
    int NumDirectionalLights;

    // other global stuff
    float Exposure;
    float AspectRatio;
    float HalfVFovRadians;
    int ToneMappingOption;
    int UseEnvironmentMap;
};

layout (set = 0, binding = 0) uniform _ViewInfoUBO
{
    ViewInfo data;
}
_ViewInfo;

ViewInfo GetViewInfo() {
    return _ViewInfo.data;
}

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

layout (set = 0, binding = 7) uniform sampler2D EnvironmentMap;