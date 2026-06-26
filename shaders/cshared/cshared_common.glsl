#ifndef NIAR_CSHARED_COMMON_GLSL
#define NIAR_CSHARED_COMMON_GLSL


#define DSET_FRAMEGLOBAL 0
#define DSET_INDEPENDENT 1
#define DSET_BINDLESS 2
#define DSET_DYNAMIC 3

#define MAX_BINDLESS_TEXTURES_2D 1024u
#define INVALID_BINDLESS_INDEX 0xffffffffu


#ifdef __cplusplus

#include <cstddef>
#include <glm/glm.hpp>
#define CSHARED_ALIGNAS_16 alignas(16)
namespace glm {

#else
#define CSHARED_ALIGNAS_16
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

struct CSHARED_ALIGNAS_16 GpuMaterial
{
	vec4 baseColorFactor;
	vec4 emissiveFactorAndClipThreshold;
	vec4 ormAndNormalStrength;
	uvec4 textureIndices; // albedo, normal, orm, emissive
};

#undef CSHARED_ALIGNAS_16

#ifdef __cplusplus
}; // namespace glm

static_assert(sizeof(glm::GpuMaterial) == 64);
static_assert(alignof(glm::GpuMaterial) == 16);
static_assert(offsetof(glm::GpuMaterial, baseColorFactor) == 0);
static_assert(offsetof(glm::GpuMaterial, emissiveFactorAndClipThreshold) == 16);
static_assert(offsetof(glm::GpuMaterial, ormAndNormalStrength) == 32);
static_assert(offsetof(glm::GpuMaterial, textureIndices) == 48);

#endif // #ifdef __cplusplus

#endif // #ifndef NIAR_CSHARED_COMMON_GLSL