#pragma once
#include "Engine/SceneObject.hpp"
#include "Scene/AABB.hpp"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vertex.h"
#include <unordered_map>

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

struct Mesh : SceneObject {
public:

	static std::vector<Mesh*> LoadMeshes(const std::string& source, bool initialize_graphics = true);

	// load from assimp data
	explicit Mesh(
		aiMesh* mesh,
		SceneObject* _parent = nullptr,
		std::string _name = "[unnamed mesh]");

	// load from glTF data
	static std::vector<Mesh*> load_gltf(
		const std::string& node_name,
		const tinygltf::Mesh* in_mesh,
		const tinygltf::Model* in_model,
		const std::vector<std::string>& texture_names);

	void initialize_gpu();
	~Mesh() override;

	void update(float elapsed) override;

	void draw(VkCommandBuffer cmdbuf) override;

	void set_local_position(glm::vec3 _local_position) override;
	void set_rotation(glm::quat _rotation) override;
	void set_scale(glm::vec3 _scale) override;

	std::vector<Vertex> vertices;
	std::vector<VERTEX_INDEX_TYPE> faces;
	uint32_t get_num_triangles() const { return faces.size() / 3; }

	AABB aabb;
	BSDF* bsdf = nullptr;

	std::string materialName;

	static void set_material_name(const std::string& mesh_name, const std::string& mat_name);

private:

	static std::unordered_map<std::string, std::string> material_assignment;

	explicit Mesh(
		const std::string& name,
		const tinygltf::Primitive* in_mesh,
		const tinygltf::Model* in_model,
		const std::vector<std::string>& material_names);

	bool locked = false;
	void generate_aabb();

	//---- vulkan stuff ----

	VmaBuffer vertexBuffer;
	VmaBuffer indexBuffer;

	void create_vertex_buffer();
	void create_index_buffer();

};
