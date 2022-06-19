#include "Mesh.h"
#include "Assets/ConfigAsset.hpp"

#if GRAPHICS_DISPLAY
#include "Render/Materials/Material.h"
#endif

using namespace glm;

std::unordered_map<std::string, std::string> Mesh::material_assignment;

void Mesh::set_material_name(const std::string& mesh_name, const std::string& mat_name) {
	material_assignment[mesh_name] = mat_name;
}

#if GRAPHICS_DISPLAY
void Mesh::draw(VkCommandBuffer cmdbuf)
{
	// TODO: might be able to get rid of the binding calls by specifying offsets in just the draw call?
	auto vb = gpu_data.vertexBuffer->getBufferInstance();
	vkCmdBindVertexBuffers(cmdbuf, 0, 1, &vb, &gpu_data.vertexBufferOffsetBytes);
	vkCmdBindIndexBuffer(cmdbuf, gpu_data.indexBuffer->getBufferInstance(), gpu_data.indexBufferOffsetBytes, VK_INDEX_TYPE);
	vkCmdDrawIndexed(cmdbuf, get_num_indices(), 1, 0, 0, 0);
}
#endif

Mesh::Mesh(const std::string &in_name,
		   const std::string &in_material_name)
{
	name = in_name;
	if (in_material_name.length() > 0) {
		materialName = in_material_name;
		set_material_name(in_name, in_material_name);
	}
}

const Vertex *Mesh::get_vertices() const
{
	return &(*cpu_data.vertices)[cpu_data.offset_num_vertices];
}

uint32_t Mesh::get_num_vertices() const
{
	return cpu_data.num_vertices;
}

const VERTEX_INDEX_TYPE *Mesh::get_indices() const
{
	return &(*cpu_data.faces)[cpu_data.offset_num_indices];
}

uint32_t Mesh::get_num_indices() const
{
	return cpu_data.num_indices;
}
