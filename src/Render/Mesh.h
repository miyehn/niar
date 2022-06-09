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

	/*
	 * Should only be called from gltf loader
	 */
	explicit Mesh(
		const std::string& name,
		const tinygltf::Primitive* in_mesh,
		const tinygltf::Model* in_model,
		const std::vector<std::string>& material_names);

	~Mesh();

#if GRAPHICS_DISPLAY
	void initialize_gpu();

	void draw(VkCommandBuffer cmdbuf);
#endif

	std::string name;

	std::vector<Vertex> vertices;
	std::vector<VERTEX_INDEX_TYPE> faces;
	uint32_t get_num_triangles() const { return faces.size() / 3; }

	std::string materialName;

	static void set_material_name(const std::string& mesh_name, const std::string& mat_name);

private:

	static std::unordered_map<std::string, std::string> material_assignment;

	bool locked = false;

#if GRAPHICS_DISPLAY
	//---- vulkan stuff ----

	VmaBuffer vertexBuffer;
	VmaBuffer indexBuffer;

	void create_vertex_buffer();
	void create_index_buffer();
#endif

};
