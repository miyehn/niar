#pragma once
#include "Engine/SceneObject.hpp"
#include "Scene/AABB.hpp"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vertex.h"

struct BSDF;
struct aiMesh;
class Material;

struct Mesh : SceneObject {

	static std::vector<Mesh*> LoadMeshes(const std::string& source, bool initialize_graphics = true);

	explicit Mesh(
		aiMesh* mesh,
		SceneObject* _parent = nullptr,
		std::string _name = "[unnamed mesh]");
	void initialize_gpu();
	~Mesh() override;

	bool handle_event(SDL_Event event) override;
	void update(float elapsed) override;

	void draw(VkCommandBuffer cmdbuf);

	void set_local_position(glm::vec3 _local_position) override;
	void set_rotation(glm::quat _rotation) override;
	void set_scale(glm::vec3 _scale) override;

	std::vector<Vertex> vertices;
	std::vector<uint16_t> faces;
	uint32_t get_num_triangles() const { return faces.size() / 3; }

	AABB aabb;
	BSDF* bsdf = nullptr;

	// Vulkan
	Material* material = nullptr;

	static void set_material_name_for(const std::string& mesh_name, const std::string& mat_name);

private:

	static std::string get_material_name_for(const std::string& mesh_name);
	static std::unordered_map<std::string, std::string> material_assignment;

	bool is_thin_mesh;
	bool locked = false;
	void generate_aabb();

	//---- vulkan stuff ----

	VmaBuffer vertexBuffer;
	VmaBuffer indexBuffer;

	void create_vertex_buffer();
	void create_index_buffer();

};
