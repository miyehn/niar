#ifndef NIAR_CSHARED_COMMON_GLSL
#define NIAR_CSHARED_COMMON_GLSL


#define DSET_FRAMEGLOBAL 0
#define DSET_INDEPENDENT 1
#define DSET_BINDLESS 2

#define MAX_BINDLESS_TEXTURES_2D 1024u
#define INVALID_BINDLESS_INDEX 0xffffffffu
#define INVALID_SCENE_GEOMETRY_INDEX 0xffffffffu
#define GPU_GEOMETRY_INDEX_TYPE_UINT16 0u
#define GPU_GEOMETRY_INDEX_TYPE_UINT32 1u
#define GPU_GEOMETRY_INDEX_TYPE_INVALID 0xffffffffu

#define GLTF_MODEL_MATRIX_PUSH_OFFSET 0
#define GLTF_MODEL_MATRIX_PUSH_SIZE 64
#define GLTF_MATERIAL_INDEX_PUSH_OFFSET 64
#define GLTF_MATERIAL_INDEX_PUSH_SIZE 4


#ifdef __cplusplus

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#define CSHARED_ALIGNAS_16 alignas(16)
namespace glm {

#else
#define CSHARED_ALIGNAS_16
#endif // #ifdef __cplusplus

struct CSHARED_ALIGNAS_16 ViewInfo
{
	mat4 ViewMatrix;
	mat4 ProjectionMatrix; // jittered
	mat4 InverseProjectionMatrix; // jittered
	mat4 PrevViewMatrix;

	// unjittered current-frame projection, kept only for motion vector computation;
	// ProjectionMatrix/InverseProjectionMatrix above carry the jitter for rendering + reconstruction
	mat4 UnjitteredProjectionMatrix;
	mat4 PrevUnjitteredProjectionMatrix;

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
	vec2 JitterOffset;
};

struct CSHARED_ALIGNAS_16 GpuMaterial
{
	vec4 baseColorFactor;
	vec4 emissiveFactorAndClipThreshold;
	vec4 ormAndNormalStrength;
	uvec4 textureIndices; // albedo, normal, orm, emissive
};

struct CSHARED_ALIGNAS_16 GpuSceneInstanceRecord
{
	uint bindlessMaterialIndex;
	uint geometryRecordIndex;
	uvec2 reserved; // reserved for scene lookup metadata without changing stride
};

struct CSHARED_ALIGNAS_16 GpuGeometryRecord
{
	uvec2 vertexBufferAddress;
	uint vertexBufferOffsetBytes;
	uint vertexCount;

	uint vertexStride;
	uint indexType;
	uvec2 _pad0;

	uint positionAttribOffset;
	uint normalAttribOffset;
	uint tangentAttribOffset;
	uint uvAttribOffset;

	uvec2 indexBufferAddress;
	uint indexBufferOffsetBytes;
	uint indexCount;
};

#undef CSHARED_ALIGNAS_16

#ifdef __cplusplus
}; // namespace glm

static_assert(sizeof(glm::ViewInfo) == 480);
static_assert(alignof(glm::ViewInfo) == 16);
static_assert(offsetof(glm::ViewInfo, ViewMatrix) == 0);
static_assert(offsetof(glm::ViewInfo, ProjectionMatrix) == 64);
static_assert(offsetof(glm::ViewInfo, InverseProjectionMatrix) == 128);
static_assert(offsetof(glm::ViewInfo, PrevViewMatrix) == 192);
static_assert(sizeof(glm::GpuMaterial) == 64);
static_assert(alignof(glm::GpuMaterial) == 16);
static_assert(offsetof(glm::GpuMaterial, baseColorFactor) == 0);
static_assert(offsetof(glm::GpuMaterial, emissiveFactorAndClipThreshold) == 16);
static_assert(offsetof(glm::GpuMaterial, ormAndNormalStrength) == 32);
static_assert(offsetof(glm::GpuMaterial, textureIndices) == 48);
static_assert(sizeof(glm::GpuSceneInstanceRecord) == 16);
static_assert(alignof(glm::GpuSceneInstanceRecord) == 16);
static_assert(offsetof(glm::GpuSceneInstanceRecord, bindlessMaterialIndex) == 0);
static_assert(offsetof(glm::GpuSceneInstanceRecord, geometryRecordIndex) == 4);
static_assert(offsetof(glm::GpuSceneInstanceRecord, reserved) == 8);
static_assert(sizeof(glm::GpuGeometryRecord) == 64);
static_assert(alignof(glm::GpuGeometryRecord) == 16);
static_assert(offsetof(glm::GpuGeometryRecord, vertexBufferAddress) == 0);
static_assert(offsetof(glm::GpuGeometryRecord, vertexBufferOffsetBytes) == 8);
static_assert(offsetof(glm::GpuGeometryRecord, vertexCount) == 12);
static_assert(offsetof(glm::GpuGeometryRecord, vertexStride) == 16);
static_assert(offsetof(glm::GpuGeometryRecord, indexType) == 20);
static_assert(offsetof(glm::GpuGeometryRecord, _pad0) == 24);
static_assert(offsetof(glm::GpuGeometryRecord, positionAttribOffset) == 32);
static_assert(offsetof(glm::GpuGeometryRecord, normalAttribOffset) == 36);
static_assert(offsetof(glm::GpuGeometryRecord, tangentAttribOffset) == 40);
static_assert(offsetof(glm::GpuGeometryRecord, uvAttribOffset) == 44);
static_assert(offsetof(glm::GpuGeometryRecord, indexBufferAddress) == 48);
static_assert(offsetof(glm::GpuGeometryRecord, indexBufferOffsetBytes) == 56);
static_assert(offsetof(glm::GpuGeometryRecord, indexCount) == 60);
static_assert(GLTF_MODEL_MATRIX_PUSH_OFFSET == 0);
static_assert(GLTF_MODEL_MATRIX_PUSH_SIZE == sizeof(glm::mat4));
static_assert(GLTF_MATERIAL_INDEX_PUSH_OFFSET == sizeof(glm::mat4));
static_assert(GLTF_MATERIAL_INDEX_PUSH_SIZE == sizeof(uint32_t));

#endif // #ifdef __cplusplus

#endif // #ifndef NIAR_CSHARED_COMMON_GLSL
