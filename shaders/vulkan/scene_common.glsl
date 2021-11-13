layout (set = 0, binding = 0) uniform _ViewInfoUBO
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
}
_ViewInfo;

struct ViewInfo
{
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;

    vec3 CameraPosition;
    vec3 ViewDir;

    int NumPointLights;
    int NumDirectionalLights;

    float Exposure;
    int ToneMappingOption;
};

ViewInfo GetViewInfo()
{
    ViewInfo info;
    info.ViewMatrix = _ViewInfo.ViewMatrix;
    info.ProjectionMatrix = _ViewInfo.ProjectionMatrix;
    info.CameraPosition = _ViewInfo.CameraPosition;
    info.NumPointLights = _ViewInfo.NumPointLights;
    info.ViewDir = _ViewInfo.ViewDir;
    info.NumDirectionalLights = _ViewInfo.NumDirectionalLights;
    info.Exposure = _ViewInfo.Exposure;
    info.ToneMappingOption = _ViewInfo.ToneMappingOption;
    return info;
}