#include "Mesh.h"
#include "Pathtracer/BSDF.hpp"
#include "Engine/ConfigAsset.hpp"

#include <tinygltf/tiny_gltf.h>
#include <map>

#if GRAPHICS_DISPLAY
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/Materials/Material.h"
#endif

using namespace glm;

std::unordered_map<std::string, std::string> Mesh::material_assignment;

void Mesh::set_material_name(const std::string& mesh_name, const std::string& mat_name) {
	material_assignment[mesh_name] = mat_name;
}

#if GRAPHICS_DISPLAY
void Mesh::create_vertex_buffer()
{
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	// create a staging buffer
	VmaBuffer stagingBuffer(&Vulkan::Instance->memoryAllocator, bufferSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// copy vertex buffer memory over to staging buffer
	stagingBuffer.writeData(vertices.data());

	// now create the actual vertex buffer
	auto vkUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vertexBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator, bufferSize, vkUsage, VMA_MEMORY_USAGE_GPU_ONLY);

	// and copy stuff from staging buffer to vertex buffer
	vk::copyBuffer(vertexBuffer.getBufferInstance(), stagingBuffer.getBufferInstance(), bufferSize);
	stagingBuffer.release();
}

void Mesh::create_index_buffer()
{
	VkDeviceSize bufferSize = sizeof(VERTEX_INDEX_TYPE) * faces.size();

	// create a staging buffer
	VmaBuffer stagingBuffer(&Vulkan::Instance->memoryAllocator, bufferSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// copy data to staging buffer
	stagingBuffer.writeData(faces.data());

	// create the actual index buffer
	auto vkUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	indexBuffer = VmaBuffer(&Vulkan::Instance->memoryAllocator, bufferSize, vkUsage, VMA_MEMORY_USAGE_GPU_ONLY);
	NAME_OBJECT(VK_OBJECT_TYPE_BUFFER, indexBuffer.getBufferInstance(), "index buffer: " + name)

	// move stuff from staging buffer and destroy staging buffer
	vk::copyBuffer(indexBuffer.getBufferInstance(), stagingBuffer.getBufferInstance(), bufferSize);
	stagingBuffer.release();
}

void Mesh::initialize_gpu() {

	create_vertex_buffer();
	create_index_buffer();

	LOG("loaded mesh '%s' of %lu vertices and %d triangles", name.c_str(), vertices.size(), get_num_triangles());
}

void Mesh::update(float elapsed) {
	SceneObject::update(elapsed);
	locked = true;
}

void Mesh::draw(VkCommandBuffer cmdbuf)
{
	//get_material()->setParameters(cmdbuf, this);
	VkDeviceSize offsets[] = { 0 };
	auto vb = vertexBuffer.getBufferInstance();
	vkCmdBindVertexBuffers(cmdbuf, 0, 1, &vb, offsets); // offset, #bindings, (content)
	vkCmdBindIndexBuffer(cmdbuf, indexBuffer.getBufferInstance(), 0, VK_INDEX_TYPE);
	vkCmdDrawIndexed(cmdbuf, faces.size(), 1, 0, 0, 0);
}
#endif

void Mesh::generate_aabb() {
	mat4 o2w = object_to_world();
	aabb = AABB();
	for (int i=0; i<vertices.size(); i++) {
		aabb.add_point(o2w * vec4(vertices[i].position, 1));
	}
}

Mesh::~Mesh() {
	//if (bsdf) delete bsdf;
#if GRAPHICS_DISPLAY
	vertexBuffer.release();
	indexBuffer.release();
#endif
}

void Mesh::set_local_position(vec3 local_position) {
	if (!locked) {
		_local_position = local_position;
		generate_aabb();
		//get_scene()->generate_aabb();
	}
}

void Mesh::set_rotation(quat rotation) {
	if (!locked) {
		_rotation = rotation;
		generate_aabb();
		//get_scene()->generate_aabb();
	}
}

void Mesh::set_scale(vec3 scale) {
	if (!locked) {
		_scale = scale;
		generate_aabb();
		//get_scene()->generate_aabb();
	}
}

// for loading tinygltf only
Mesh::Mesh(
	const std::string& in_name,
	const tinygltf::Primitive *in_prim,
	const tinygltf::Model *in_model,
	const std::vector<std::string>& material_names)
{
	name = in_name;
	auto get_data = [&](
		int accessor_idx,
		const uint8_t** out_data,
		uint32_t* out_size,
		uint32_t* num_components=nullptr,
		uint32_t* component_type=nullptr)
	{
		auto accessor = in_model->accessors[accessor_idx];
		auto buffer_view = in_model->bufferViews[accessor.bufferView];
		*out_data = &in_model->buffers[buffer_view.buffer].data[buffer_view.byteOffset + accessor.byteOffset];
		*out_size = accessor.count;
		if (num_components) *num_components = accessor.type;
		if (component_type) *component_type = accessor.componentType;
	};

	// vertices
	const vec3* positions;
	const vec3* normals;
	const vec3* tangents;
	const vec2* uvs;
	uint32_t positions_cnt, normals_cnt, tangents_cnt, uvs_cnt;
	// TODO: can check for data types for safety (now assuming correct #components; all floats)
	get_data(in_prim->attributes.at("POSITION"), reinterpret_cast<const uint8_t**>(&positions), &positions_cnt);
	get_data(in_prim->attributes.at("NORMAL"), reinterpret_cast<const uint8_t**>(&normals), &normals_cnt);
	get_data(in_prim->attributes.at("TANGENT"), reinterpret_cast<const uint8_t**>(&tangents), &tangents_cnt);
	get_data(in_prim->attributes.at("TEXCOORD_0"), reinterpret_cast<const uint8_t**>(&uvs), &uvs_cnt);

	EXPECT_M(positions_cnt == normals_cnt && normals_cnt == tangents_cnt && tangents_cnt == uvs_cnt, true,
			 "Mesh prims should have the same number of each attribute!");

	for (auto i = 0; i < positions_cnt; i++)
	{
		Vertex v;
		v.position = positions[i];
		v.normal = normals[i];
		v.tangent = tangents[i];
		v.uv = uvs[i];
		vertices.push_back(v);
	}

	// faces
	const uint8_t* indices_data;
	uint32_t indices_cnt, num_components, component_type;
	get_data(in_prim->indices, &indices_data, &indices_cnt, &num_components, &component_type);
	EXPECT_M(component_type, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, "Indices are not 16-bit unsigned ints!")
	EXPECT_M(indices_cnt % 3, 0, "Num indices is not a multiply of 3!")

	faces.resize(indices_cnt);
	memcpy(faces.data(), indices_data, indices_cnt * sizeof(VERTEX_INDEX_TYPE));

	generate_aabb();

	uint32_t mat_index = in_prim->material >= 0 ? in_prim->material : 0;
	materialName = material_names[mat_index];
	set_material_name(name, material_names[mat_index]);
}
