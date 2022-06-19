#pragma once
#include "Scene/SceneObject.hpp"
#include "Scene/AABB.hpp"
#include "Render/Vertex.h"
#include <unordered_map>
#if GRAPHICS_DISPLAY
#include "Render/Vulkan/Buffer.h"
#endif

struct BSDF;
struct aiMesh;
class Material;

namespace tinygltf
{
	struct Mesh;
	struct Primitive;
	struct Model;
}

#define VERTEX_INDEX_TYPE uint16_t
#define VK_INDEX_TYPE VK_INDEX_TYPE_UINT16

struct Mesh {
public:

	explicit Mesh(
		const std::string& name,
		const std::string& material_name);

	~Mesh() = default;

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
	};

	void draw(VkCommandBuffer cmdbuf);
#endif

	std::string name;

	[[nodiscard]] const Vertex* get_vertices() const;
	[[nodiscard]] uint32_t get_num_vertices() const;

	[[nodiscard]] const VERTEX_INDEX_TYPE* get_indices() const;
	[[nodiscard]] uint32_t get_num_indices() const;

	std::string materialName;

	CpuDataAccessor cpu_data{};
#if GRAPHICS_DISPLAY
	GpuDataAccessor gpu_data{};
#endif

	static void set_material_name(const std::string& mesh_name, const std::string& mat_name);

private:

	static std::unordered_map<std::string, std::string> material_assignment;

	bool locked = false;
};
