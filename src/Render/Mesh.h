#pragma once
#include "Scene/SceneObject.hpp"
#include "Render/Vertex.h"
#include <unordered_map>
#if GRAPHICS_DISPLAY
#include "cshared_common.h"
#include "Render/Vulkan/Buffer.h"
#endif

struct BSDF;
struct aiMesh;

/*

glTF material data is represented by GpuMaterial on the GPU, or GltfMaterialInfo on the CPU.
They contain only data and should be generic enough for any consumer of glTF surface renderer, rasterization or not.
- the only separation for cpu/gpu is because gpu uses bindless texture table but cpu doesn't

Subpasses are "function signatures". They describe input & output count and their formats
Render passes are like graphs connecting subpasses and specifying dependencies.

Pipelines are "function bodies" that implement a subpass "signature" (w other constraints: dset layout & push constants)
Pipelines consist of:
fixed-function operations (blend op, cull mode, depth test, etc.) and programmed behaviors (shaders)
- Pipelines accessing material data through descriptors is like functions reading from external (global) variables.

The next cleanup should separate material data (assume glTF surface material for now) and pipeline selection.
Pipeline selection should be based on functionality which does come from material,
but not all material data decides functionality. Some data are just pure data (which specific texture to use etc.)
For this repo, pipeline selection cares about:
- opaque? (decides which pass/stage as well as alpha behavior)
- cull back face?
- (maybe) vertex layout

In the future there could also be non-surface materials that use data blobs different from GltfMaterialInfo.
For example volume materials, particle (VFX) materials. Will deal with them later.

*/

namespace tinygltf
{
	struct Mesh;
	struct Primitive;
	struct Model;
}

#define VERTEX_INDEX_TYPE uint16_t
#define VK_INDEX_TYPE VK_INDEX_TYPE_UINT16

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
	};

	void draw(VkCommandBuffer cmdbuf);
#endif

	std::string name;

	[[nodiscard]] const Vertex* get_vertices() const;
	[[nodiscard]] uint32_t get_num_vertices() const;

	[[nodiscard]] const VERTEX_INDEX_TYPE* get_indices() const;
	[[nodiscard]] uint32_t get_num_indices() const;

	std::string materialName;
#if GRAPHICS_DISPLAY
	// Migration bridge: SceneAsset fills this from its glTF material-index mapping.
	// Future bindless draw paths can read it per mesh without consulting GltfMaterialInfo.
	uint32_t bindlessMaterialIndex = INVALID_BINDLESS_INDEX;
#endif

	CpuDataAccessor cpu_data{};
#if GRAPHICS_DISPLAY
	GpuDataAccessor gpu_data{};
#endif

	static void set_material_name(const std::string& mesh_name, const std::string& mat_name);

private:

	static std::unordered_map<std::string, std::string> material_assignment;

	bool locked = false;
};
