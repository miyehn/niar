#include "Mesh.h"

#if GRAPHICS_DISPLAY
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/Vertex.h"
#endif

using namespace glm;

std::unordered_map<std::string, std::string> Mesh::material_assignment;

Mesh::~Mesh()
{
#if GRAPHICS_DISPLAY
	if (gpu_data.blas != VK_NULL_HANDLE)
		Vulkan::Instance->fn_vkDestroyAccelerationStructureKHR(Vulkan::Instance->device, gpu_data.blas, nullptr);
	gpu_data.blasBuffer.release();
#endif
}

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

void Mesh::build_blas()
{
	VkBufferDeviceAddressInfo vbAddrInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = gpu_data.vertexBuffer->getBufferInstance()
	};
	VkDeviceAddress vbAddr = vkGetBufferDeviceAddress(Vulkan::Instance->device, &vbAddrInfo);

	VkBufferDeviceAddressInfo ibAddrInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = gpu_data.indexBuffer->getBufferInstance()
	};
	VkDeviceAddress ibAddr = vkGetBufferDeviceAddress(Vulkan::Instance->device, &ibAddrInfo);

	VkAccelerationStructureGeometryTrianglesDataKHR triangles = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
		.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
		.vertexData = {.deviceAddress = vbAddr + gpu_data.vertexBufferOffsetBytes},
		.vertexStride = sizeof(Vertex),
		.maxVertex = cpu_data.num_vertices - 1,
		.indexType = VK_INDEX_TYPE,
		.indexData = {.deviceAddress = ibAddr + gpu_data.indexBufferOffsetBytes},
	};
	VkAccelerationStructureGeometryKHR geom = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
		.geometry = {.triangles = triangles},
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
	};
	VkAccelerationStructureBuildRangeInfoKHR range = {
		.primitiveCount = cpu_data.num_indices / 3,
		.primitiveOffset = 0,
		.firstVertex = 0,
		.transformOffset = 0
	};

	vk::build_blas(
		geom, range,
		VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR |
		VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		&gpu_data.blas,
		&gpu_data.blasBuffer);
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
