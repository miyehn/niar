#pragma once
#include "Engine/Drawable.hpp"
#include "Utils/Utils.hpp"
// #include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/Buffer.h"
#include "Vertex.h"

#define NUM_MATERIAL_SETS 3

struct BSDF;
struct aiMesh;
struct GlTexture;
struct GlMaterial;
class Material;

struct Mesh : Drawable {

	static std::vector<Mesh*> LoadMeshes(const std::string& source, bool initialize_graphics = true);

	Mesh(
		aiMesh* mesh,
		Drawable* _parent = nullptr,
		std::string _name = "[unnamed mesh]");
	Mesh();
	void initialize_gpu();
	virtual ~Mesh();

	virtual bool handle_event(SDL_Event event) override;
	virtual void update(float elapsed) override;
	virtual void draw() override;

	void draw(VkCommandBuffer cmdbuf);

	virtual void set_local_position(vec3 _local_position) override;
	virtual void set_rotation(quat _rotation) override;
	virtual void set_scale(vec3 _scale) override;

	std::vector<Vertex> vertices;
	std::vector<uint16_t> faces;
	uint get_num_triangles() { return faces.size() / 3; }

	AABB aabb;
	BSDF* bsdf = nullptr;

	GlMaterial* materials[NUM_MATERIAL_SETS];

	// Vulkan
	Material* material = nullptr;

	static void set_material_name_for(const std::string& mesh_name, const std::string& mat_name);

private:

	static std::string get_material_name_for(const std::string& mesh_name);
	static std::unordered_map<std::string, std::string> material_assignment;

	bool is_thin_mesh;
	bool locked = false;
	void generate_aabb();

	//---- opengl stuff ----
	uint vbo, ebo, vao = 0;

	//---- vulkan stuff ----

	VmaBuffer vertexBuffer;
	VmaBuffer indexBuffer;

	void create_vertex_buffer();
	void create_index_buffer();

};
