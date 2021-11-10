#include "Mesh.h"
#include "Pathtracer/BSDF.hpp"
#include "Engine/Config.hpp"

#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/VulkanUtils.h"

#include "Render/Materials/Material.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <tinygltf/tiny_gltf.h>
#include <map>

#define VK_INDEX_TYPE VK_INDEX_TYPE_UINT16

using namespace glm;

std::unordered_map<std::string, std::string> Mesh::material_assignment;

void Mesh::set_material_name(const std::string& mesh_name, const std::string& mat_name) {
	material_assignment[mesh_name] = mat_name;
}

Mesh::Mesh(aiMesh* mesh, SceneObject* _parent, std::string _name) : SceneObject(_parent, _name) {

	if (!mesh->HasPositions() || !mesh->HasFaces() || !mesh->HasNormals()) {
		ERR("creating a mesh that has some data missing. skipping..");
		return;
	}

	// name
	const char* inName = mesh->mName.C_Str();
	if (_name == "[unnamed mesh]") name = inName;

	// iterate through vertices
	for (int i=0; i<mesh->mNumVertices; i++) {
		// access p, n, t data
		aiVector3D position = mesh->mVertices[i];
		aiVector3D normal = mesh->mNormals[i];
		aiVector3D uv = mesh->mTextureCoords[0][i];
		aiVector3D tangent = mesh->mTangents[i];
		// create vertex from pnc
		Vertex v;
		v.position = vec3(position.x, position.y, position.z);
		v.normal = vec3(normal.x, normal.y, normal.z);
		v.tangent = vec3(tangent.x, tangent.y, tangent.z);
		v.tangent = normalize(v.tangent - dot(v.tangent, v.normal) * v.normal); // gram-schmidt
		v.uv = vec2(uv.x, uv.y);
		vertices.push_back(v);
	}

	// iterate through faces indices and store them
	for (int j=0; j<mesh->mNumFaces; j++) {
		aiFace face = mesh->mFaces[j];
		VERTEX_INDEX_TYPE i1 = face.mIndices[0];
		VERTEX_INDEX_TYPE i2 = face.mIndices[1];
		VERTEX_INDEX_TYPE i3 = face.mIndices[2];
		faces.push_back(i1);
		faces.push_back(i2);
		faces.push_back(i3);
	}

	generate_aabb();
}

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

	// move stuff from staging buffer and destroy staging buffer
	vk::copyBuffer(indexBuffer.getBufferInstance(), stagingBuffer.getBufferInstance(), bufferSize);
	stagingBuffer.release();
}

void Mesh::initialize_gpu() {

	create_vertex_buffer();
	create_index_buffer();

	LOG("loaded mesh '%s' of %lu vertices and %d triangles", name.c_str(), vertices.size(), get_num_triangles());
}

void Mesh::generate_aabb() {
	mat4 o2w = object_to_world();
	aabb = AABB();
	for (int i=0; i<vertices.size(); i++) {
		aabb.add_point(o2w * vec4(vertices[i].position, 1));
	}
}

Mesh::~Mesh() {
	if (bsdf) delete bsdf;
	vertexBuffer.release();
	indexBuffer.release();
}

void Mesh::update(float elapsed) {
	SceneObject::update(elapsed);
	locked = true;
}

void Mesh::draw(VkCommandBuffer cmdbuf)
{
	get_material()->setParameters(this);

	VkDeviceSize offsets[] = { 0 };
	auto vb = vertexBuffer.getBufferInstance();
	vkCmdBindVertexBuffers(cmdbuf, 0, 1, &vb, offsets); // offset, #bindings, (content)
	vkCmdBindIndexBuffer(cmdbuf, indexBuffer.getBufferInstance(), 0, VK_INDEX_TYPE);
	vkCmdDrawIndexed(cmdbuf, faces.size(), 1, 0, 0, 0);
}

void Mesh::set_local_position(vec3 _local_position) {
	if (!locked) {
		local_position_value = _local_position;
		generate_aabb();
		//get_scene()->generate_aabb();
	}
}

void Mesh::set_rotation(quat _rotation) {
	if (!locked) {
		rotation_value = _rotation;
		generate_aabb();
		//get_scene()->generate_aabb();
	}
}

void Mesh::set_scale(vec3 _scale) {
	if (!locked) {
		scale_value = _scale;
		generate_aabb();
		//get_scene()->generate_aabb();
	}
}

std::vector<Mesh*> Mesh::LoadMeshes(const std::string& source, bool initialize_graphics) {
	// import mesh from source
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(source,
			aiProcess_GenSmoothNormals |
			aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType);
	if (!scene) {
		ERR("%s", importer.GetErrorString());
	}

	// access and create mesh drawables from imported source
	std::vector<Mesh*> meshes;
	for (int i=0; i<scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		Mesh* sceneMesh = new Mesh(mesh);
		if (initialize_graphics) sceneMesh->initialize_gpu();
		if (mesh) meshes.push_back(sceneMesh);
	}

	LOG("loaded %lu meshe(s)", meshes.size());

	// importer seems to automatically handle memory release for scene
	return meshes;

}

Material *Mesh::get_material()
{
	auto pair = material_assignment.find(name);
	if (pair == material_assignment.end()) {
		WARN("trying to get material for mesh '%s' but it doesn't have one", name.c_str())
		return nullptr;
	}
	auto mat_name =  pair->second;
	return Material::find(mat_name);
}

std::vector<Mesh *> Mesh::load_gltf(
	const tinygltf::Mesh* in_mesh,
	const tinygltf::Model* in_model)
{
	std::vector<Mesh*> output;
	LOG("loading %s with %d primitives..", in_mesh->name.c_str(), (int)in_mesh->primitives.size())
	for (auto &prim : in_mesh->primitives)
	{
		if (prim.mode != TINYGLTF_MODE_TRIANGLES)
		{
			WARN("%s contains unsupported mesh mode %d. skipping..", in_mesh->name.c_str(), prim.mode)
			continue;
		}
		output.emplace_back(new Mesh(&prim, in_model));
	}
	return output;
}

// internal
Mesh::Mesh(
	const tinygltf::Primitive *in_prim,
	const tinygltf::Model *in_model)
{
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

	// TODO: assign material
}