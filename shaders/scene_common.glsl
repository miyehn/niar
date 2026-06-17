struct ViewInfo
{
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;
    mat4 InverseProjectionMatrix;

    vec3 CameraPosition;
    int NumPointLights;

    vec3 ViewDir;
    int NumDirectionalLights;

    // other global stuff
    float Exposure;
    float AspectRatio;
    float HalfVFovRadians;
    int ToneMappingOption;
    int BackgroundOption;
    uint FrameIndex;
    vec2 RenderSize;
    vec4 FrameRandom;
};

layout (set = 0, binding = 0) uniform _ViewInfoUBO
{
    ViewInfo data;
}
_ViewInfo;

ViewInfo GetViewInfo() {
    return _ViewInfo.data;
}

vec3 reconstructWorldPositionFromDepth(vec2 screenUv, float deviceDepth, ViewInfo viewInfo)
{
    vec2 ndc = screenUv * 2.0f - 1.0f;
    vec4 viewPos = viewInfo.InverseProjectionMatrix * vec4(ndc, deviceDepth, 1.0f);
    viewPos /= viewPos.w;

    mat3 cameraToWorldRot = transpose(mat3(viewInfo.ViewMatrix));
    return cameraToWorldRot * viewPos.xyz + viewInfo.CameraPosition;
}
