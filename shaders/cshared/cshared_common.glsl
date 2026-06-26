#ifndef NIAR_CSHARED_COMMON_GLSL
#define NIAR_CSHARED_COMMON_GLSL


#define DSET_FRAMEGLOBAL 0
#define DSET_INDEPENDENT 1
#define DSET_BINDLESS 2
#define DSET_DYNAMIC 3

#define MAX_BINDLESS_TEXTURES_2D 1024u
#define INVALID_BINDLESS_INDEX 0xffffffffu


#ifdef __cplusplus
#include <glm/glm.hpp>
namespace glm {
#endif // #ifdef __cplusplus

struct ViewInfo
{
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
	mat4 InverseProjectionMatrix;

	vec3 CameraPosition;
	int NumPointLights;

	vec3 ViewDir;
	int NumDirectionalLights;

	float Exposure;
	float AspectRatio;
	float HalfVFovRadians;
	int ToneMappingOption;
	int BackgroundOption;
	uint FrameIndex;
	vec2 RenderSize;
	vec4 FrameRandom;
};

#ifdef __cplusplus
}; // namespace glm
#endif // #ifdef __cplusplus

#endif // #ifndef NIAR_CSHARED_COMMON_GLSL