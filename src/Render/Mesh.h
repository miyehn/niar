#pragma once
#include "Scene/SceneObject.hpp"
#include "Render/Materials/GltfMaterialInfo.h"
#include "Render/Vertex.h"
#if GRAPHICS_DISPLAY
#include "cshared_common.h"
#include "Render/Vulkan/Buffer.h"
#endif

struct BSDF;
struct aiMesh;


namespace tinygltf
{
	struct Primitive;
	struct Model;
}

#define VERTEX_INDEX_TYPE uint16_t
#define VK_INDEX_TYPE VK_INDEX_TYPE_UINT16

struct MeshSurface {
	std::string materialName;

#if GRAPHICS_DISPLAY
	uint32_t bindlessMaterialIndex = INVALID_BINDLESS_INDEX;
	BlendMode blendMode = BM_OpaqueOrClip;
	bool doubleSided = false;

	bool isOpaque() const { return blendMode == BM_OpaqueOrClip; }
#endif
};

// is really just a mesh instance; it doesn't own anything. Assets own the data.
struct Mesh {

	Mesh() = default;
	~Mesh() = default;

	explicit Mesh(
		const std::string& name,
		const std::string& material_name);

	struct CpuDataAccessor {
		const std::vector<Vertex>* vertices;
		uint32_t num_vertices;
		uint32_t offset_num_vertices;
		const std::vector<VERTEX_INDEX_TYPE>* faces;
		uint32_t num_indices;
		uint32_t offset_num_indices;
	};

#if GRAPHICS_DISPLAY

	struct GpuDataAccessor {
		const VmaBuffer* vertexBuffer;
		VkDeviceSize vertexBufferOffsetBytes;
		const VmaBuffer* indexBuffer;
		VkDeviceSize indexBufferOffsetBytes;
		VkAccelerationStructureKHR blasHandle = VK_NULL_HANDLE;
		VkDeviceAddress blasAddress = 0;
		uint32_t geometryRecordIndex = INVALID_SCENE_GEOMETRY_INDEX;
	};

	void draw(VkCommandBuffer cmdbuf);
#endif

	std::string name;

	[[nodiscard]] const Vertex* get_vertices() const;
	[[nodiscard]] uint32_t get_num_vertices() const;

	[[nodiscard]] const VERTEX_INDEX_TYPE* get_indices() const;
	[[nodiscard]] uint32_t get_num_indices() const;

	MeshSurface surface;

	CpuDataAccessor cpu_data{};
#if GRAPHICS_DISPLAY
	GpuDataAccessor gpu_data{};
#endif

};
