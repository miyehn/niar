#include "cshared/cshared_common.h"

layout (set = DSET_FRAMEGLOBAL, binding = 0) uniform _ViewInfoUBO
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
