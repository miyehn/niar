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
    int ToneMappingOption;
};

layout (set = 0, binding = 0) uniform _ViewInfoUBO
{
    ViewInfo data;
}
_ViewInfo;

ViewInfo GetViewInfo()
{
    return _ViewInfo.data;
}