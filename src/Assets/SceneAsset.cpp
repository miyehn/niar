//
// Created by raind on 5/22/2022.
//

#include "Scene/Scene.hpp"
#include "Scene/Camera.hpp"
#include "Scene/Light.hpp"
#include "Render/Mesh.h"
#include "ConfigAsset.hpp"
#include "SceneAsset.h"
#include "Render/Materials/GltfMaterialInfo.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <tinygltf/tiny_gltf.h>

#include <unordered_map>
#include <queue>
#include "Scene/MeshObject.h"

#if GRAPHICS_DISPLAY
#include "Render/Vulkan/VulkanUtils.h"
#include "Render/Texture.h"
#endif

namespace
{
struct SceneNodeIntermediate
{
	SceneNodeIntermediate() = default;
	// converter
	explicit SceneNodeIntermediate(const tinygltf::Node &in_node)
	{
		mesh_idx = in_node.mesh;
		camera_idx = in_node.camera;
		name = in_node.name;
		if (!in_node.rotation.empty())
		{
			glm::quat rot(in_node.rotation[3], in_node.rotation[0], in_node.rotation[1], in_node.rotation[2]);
			transformation = glm::mat4_cast(rot);
		}
		if (!in_node.scale.empty())
		{
			auto scl = glm::mat4(1.0f);
			scl[0][0] = in_node.scale[0];
			scl[1][1] = in_node.scale[1];
			scl[2][2] = in_node.scale[2];
			scl[3][3] = 1;
			transformation *= scl;
		}
		if (!in_node.translation.empty())
		{
			auto tran = glm::mat4(1.0f);
			tran[3] = {in_node.translation[0], in_node.translation[1], in_node.translation[2], 1};
			transformation = tran * transformation;
		}
	}

	~SceneNodeIntermediate()
	{
		for (auto c : children) delete c;
	}

	// hierarchy
	SceneNodeIntermediate* parent = nullptr;
	std::vector<SceneNodeIntermediate*> children;
	void detach_from_hierarchy()
	{
		if (!parent) return;
		for (auto p = parent->children.begin(); p != parent->children.end(); p++) {
			if (*p == this) {
				parent->children.erase(p);
				break;
			}
		}
		parent = nullptr;
	}
	void attach_to(SceneNodeIntermediate* in_parent)
	{
		EXPECT(parent == nullptr, true)
		parent = in_parent;
		parent->children.push_back(this);
	}
	void foreach(const std::function<void(SceneNodeIntermediate*)>& fn)
	{
		fn(this);
		for (auto child : children)
		{
			child->foreach(fn);
		}
	}

	// transformation
	glm::mat4x4 transformation = glm::mat4x4(1.0f);

	// data
	std::string name;
	int light_idx = -1;
	int mesh_idx = -1;
	int camera_idx = -1;
};

SceneNodeIntermediate* loadSceneTree(const std::vector<tinygltf::Node> &in_nodes)
{
	auto root = new SceneNodeIntermediate();
	root->name = "gltf scene root node";
	std::vector<SceneNodeIntermediate*> nodes(in_nodes.size());
	for (int i = 0; i < in_nodes.size(); i++)
	{
		auto* node = new SceneNodeIntermediate(in_nodes[i]);
		node->attach_to(root);
		nodes[i] = node;
	}
	for (int i = 0; i < in_nodes.size(); i++)
	{
		SceneNodeIntermediate* current_node = nodes[i];
		for (auto c : in_nodes[i].children)
		{
			SceneNodeIntermediate* child = nodes[c];
			child->detach_from_hierarchy();
			child->attach_to(current_node);
		}
	}
	return root;
}

void collapseSceneTree(SceneNodeIntermediate* root)
{
	for (auto child : root->children) collapseSceneTree(child);

	if (root->parent!=nullptr &&
		root->mesh_idx == -1 &&
		root->light_idx == -1 &&
		root->camera_idx == -1 &&
		root->children.size() <= 1)
	{
		if (root->children.empty())
		{
			// transformation can't be passed to children -> try passing to parent instead
			if (root->parent->children.size() == 1)
			{
				root->parent->transformation = root->parent->transformation * root->transformation;
			}
			else
			{
				// can't be passed to parent either -> don't collapse this node
				WARN("scene contains dangling child node '%s'", root->name.c_str())
				return;
			}
		}
		else
		{
			// pass to children
			for (auto c : root->children)
			{
				c->transformation = root->transformation * c->transformation;
				c->detach_from_hierarchy();
				c->attach_to(root->parent);
			}
		}

		root->detach_from_hierarchy();
		root->children.clear();
		delete root;
	}
}

bool findMaterialProperty(const tinygltf::Material& mat, const std::string& propertyName, tinygltf::Value& out_value) {
	if (!mat.extras.IsObject()) return false;

	if (mat.extras.IsObject() && mat.extras.Has(propertyName)) {
		 out_value = mat.extras.Get(propertyName);
		 return true;
	}
	return false;
}

}// anonymous namespace

// for loading mesh buffers

struct VertexBufferIndex {
	uint32_t position_acc_idx;
	uint32_t normal_acc_idx;
	uint32_t tangent_acc_idx;
	uint32_t uv_acc_idx;
	bool operator==(const VertexBufferIndex& other) const {
		return position_acc_idx == other.position_acc_idx &&
			   normal_acc_idx == other.normal_acc_idx &&
			   tangent_acc_idx == other.tangent_acc_idx &&
			   uv_acc_idx == other.uv_acc_idx;
	}
};

struct PrimitiveBufferIndex {
	VertexBufferIndex vb_idx;
	uint32_t ib_idx;
	bool operator==(const PrimitiveBufferIndex& other) const {
		return vb_idx == other.vb_idx && ib_idx == other.ib_idx;
	}
};

namespace std {
template<> struct hash<VertexBufferIndex>
{
	std::size_t operator()(const VertexBufferIndex &idx) const noexcept {
		return (
			(hash<uint32_t>{}(idx.position_acc_idx) << 24) ^
			(hash<uint32_t>{}(idx.normal_acc_idx) << 16) ^
			(hash<uint32_t>{}(idx.tangent_acc_idx) << 8) ^
			(hash<uint32_t>{}(idx.uv_acc_idx) << 0));
	}
};
template<> struct hash<PrimitiveBufferIndex>
{
	std::size_t operator()(const PrimitiveBufferIndex &idx) const noexcept {
		return
			(hash<VertexBufferIndex>{}(idx.vb_idx)) ^
			(hash<uint32_t>{}(idx.ib_idx));
	}
};
}

inline PrimitiveBufferIndex primitive_buffer_indices(const tinygltf::Primitive& prim) {
	VertexBufferIndex vb = {
		.position_acc_idx = (uint32_t)prim.attributes.at("POSITION"),
		.normal_acc_idx = (uint32_t)prim.attributes.at("NORMAL"),
		.tangent_acc_idx = (uint32_t)prim.attributes.at("TANGENT"),
		.uv_acc_idx = (uint32_t)prim.attributes.at("TEXCOORD_0")
	};
	uint32_t ib = prim.indices;
	return {vb, ib};
}

void load_mesh_buffers(
	const tinygltf::Model& model,
	// cpu (required)
	std::unordered_map<PrimitiveBufferIndex, Mesh::CpuDataAccessor>& cpu_buffer_indices_map,
	std::vector<Vertex>& vertex_buffer_cpu,
	std::vector<VERTEX_INDEX_TYPE>& index_buffer_cpu
#if GRAPHICS_DISPLAY
	// gpu (optional)
	, std::unordered_map<PrimitiveBufferIndex, Mesh::GpuDataAccessor>* gpu_buffer_indices_map = nullptr,
	VmaBuffer* vbo = nullptr,
	VmaBuffer* ibo = nullptr
#endif
) {
	/*
	 * load all the cpu data first (append into given buffers and update map)
	 * if gpu resources are given, create them from cpu data
	 */
	using namespace glm;

	auto get_data = [&](
		int accessor_idx,
		const uint8_t** out_data,
		uint32_t* out_size,
		uint32_t* num_components=nullptr,
		uint32_t* component_type=nullptr)
	{
		auto accessor = model.accessors[accessor_idx];
		auto buffer_view = model.bufferViews[accessor.bufferView];
		*out_data = &model.buffers[buffer_view.buffer].data[buffer_view.byteOffset + accessor.byteOffset];
		*out_size = accessor.count;
		if (num_components) *num_components = accessor.type;
		if (component_type) *component_type = accessor.componentType;
	};

	// cpu
	for (auto& mesh : model.meshes) {
		for (auto& prim : mesh.primitives) {
			PrimitiveBufferIndex prim_buf_idx = primitive_buffer_indices(prim);
			if (!cpu_buffer_indices_map.contains(prim_buf_idx)) {

				uint32_t offset_num_vertices = vertex_buffer_cpu.size();
				uint32_t offset_num_indices = index_buffer_cpu.size();

				// vertices
				const vec3* positions;
				const vec3* normals;
				const vec4* tangents;
				const vec2* uvs;
				uint32_t positions_cnt, normals_cnt, tangents_cnt, uvs_cnt;
				// TODO: can check for data types for safety (now assuming correct #components; all floats)
				get_data(prim_buf_idx.vb_idx.position_acc_idx, reinterpret_cast<const uint8_t**>(&positions), &positions_cnt);
				get_data(prim_buf_idx.vb_idx.normal_acc_idx, reinterpret_cast<const uint8_t**>(&normals), &normals_cnt);
				get_data(prim_buf_idx.vb_idx.tangent_acc_idx, reinterpret_cast<const uint8_t**>(&tangents), &tangents_cnt);
				get_data(prim_buf_idx.vb_idx.uv_acc_idx, reinterpret_cast<const uint8_t**>(&uvs), &uvs_cnt);

				EXPECT_M(positions_cnt == normals_cnt && normals_cnt == tangents_cnt && tangents_cnt == uvs_cnt, true,
						 "Mesh prims should have the same number of each attribute!");

				for (auto i = 0; i < positions_cnt; i++) {
					vertex_buffer_cpu.emplace_back();
					Vertex& v = vertex_buffer_cpu.back();
					v.position = positions[i];
					v.normal = normals[i];
					v.tangent = tangents[i];
					v.uv = uvs[i];
				}

				// faces
				const VERTEX_INDEX_TYPE* indices_data;
				uint32_t indices_cnt, num_components, component_type;
				get_data(prim_buf_idx.ib_idx, reinterpret_cast<const uint8**>(&indices_data), &indices_cnt, &num_components, &component_type);
				ASSERT_M(component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, "Indices are not 16-bit unsigned ints!")
				ASSERT_M(indices_cnt % 3 == 0, "Num indices is not a multiply of 3!")

				for (auto i = 0; i < indices_cnt; i++) {
					index_buffer_cpu.push_back(indices_data[i]);
				}

				Mesh::CpuDataAccessor cpu_accessor = {
					.vertices = &vertex_buffer_cpu,
					.num_vertices = static_cast<uint32_t>(vertex_buffer_cpu.size() - offset_num_vertices),
					.offset_num_vertices = offset_num_vertices,
					.faces = &index_buffer_cpu,
					.num_indices = static_cast<uint32_t>(index_buffer_cpu.size() - offset_num_indices),
					.offset_num_indices = offset_num_indices
				};
				cpu_buffer_indices_map[prim_buf_idx] = cpu_accessor;
			}
		}
	}

#if GRAPHICS_DISPLAY
	// also load gpu resources:
	if (gpu_buffer_indices_map && vbo && ibo) {

		vk::create_vertex_buffer(
			vertex_buffer_cpu.data(), vertex_buffer_cpu.size(), sizeof(Vertex), *vbo);
		vk::create_index_buffer(
			index_buffer_cpu.data(), index_buffer_cpu.size(), sizeof(VERTEX_INDEX_TYPE), *ibo);

		for (auto& p : cpu_buffer_indices_map) {
			Mesh::GpuDataAccessor gpu_accessor = {
				.vertexBuffer = vbo,
				.vertexBufferOffsetBytes = p.second.offset_num_vertices * sizeof(Vertex),
				.indexBuffer = ibo,
				.indexBufferOffsetBytes = p.second.offset_num_indices * sizeof(VERTEX_INDEX_TYPE)
			};
			(*gpu_buffer_indices_map)[p.first] = gpu_accessor;
		}
	}
#endif
}

// load from glTF data
std::vector<Mesh*> load_gltf_meshes(
	const std::string& node_name,
	const tinygltf::Mesh* in_mesh,
	const std::vector<std::string>& material_names,
	const std::unordered_map<PrimitiveBufferIndex, Mesh::CpuDataAccessor>& cpu_buffer_indices
#if GRAPHICS_DISPLAY
	, const std::unordered_map<PrimitiveBufferIndex, Mesh::GpuDataAccessor>& gpu_buffer_indices
#endif
	)
{
	std::vector<Mesh*> output;
	LOG("loading mesh obj %s with %d primitives..", in_mesh->name.c_str(), (int)in_mesh->primitives.size())
	for (int i = 0; i < in_mesh->primitives.size(); i++)
	{
		auto& prim = in_mesh->primitives[i];
		if (prim.mode != TINYGLTF_MODE_TRIANGLES)
		{
			WARN("%s contains unsupported mesh mode %d. skipping..", in_mesh->name.c_str(), prim.mode)
			continue;
		}
		auto in_name = node_name + " | " + in_mesh->name + "[" + std::to_string(i) + "]";
		auto in_material_name = prim.material >= 0 ? material_names[prim.material] : "";

		auto buf_idx = primitive_buffer_indices(prim);
		auto m = new Mesh(in_name, in_material_name);
		m->cpu_data = cpu_buffer_indices.at(buf_idx);
#if GRAPHICS_DISPLAY
		m->gpu_data = gpu_buffer_indices.at(buf_idx);
#endif
		output.emplace_back(m);
	}
	return output;
}

SceneAsset::SceneAsset(
	SceneObject* outer_root,
	const std::string &relative_path)
: Asset(relative_path, nullptr)
{
	load_action_internal = [this, outer_root, relative_path]() {

		// cleanup first, if necessary
		if (outer_root) outer_root->try_remove_child(asset_root);
#if GRAPHICS_DISPLAY
		Vulkan::Instance->waitDeviceIdle();
#endif
		release_resources();
		delete asset_root;

		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		loader.SetPreserveImageChannels(false);
		std::string err;
		std::string warn;

		//bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, absolute_path);
		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, ROOT_DIR"/" + relative_path);

		if (!warn.empty()) WARN("[TinyGLTF] %s", warn.c_str())
		if (!err.empty()) ERR("[TinyGLTF] %s", err.c_str())
		if (!ret) ERR("[TinyGLTF] Failed to parse glTF")

		//====================

#if GRAPHICS_DISPLAY
		// image (texture), material

		struct ImageInfo {
			ImageFormat format;
			Texture2D* texture;
		};
		std::vector<ImageInfo> image_infos(model.images.size());

		// fill in format for now but defer actual creation till after mesh loading
		for (int i = 0; i < model.images.size(); i++)
		{
			auto& img = model.images[i];
			image_infos[i].format = { img.component, img.bits, 0 };
		}
		// mark albedo textures as sRGB
		for (int i = 0; i < model.materials.size(); i++)
		{
			auto& mat = model.materials[i];
			int albedo_tex_idx = mat.pbrMetallicRoughness.baseColorTexture.index;
			if (albedo_tex_idx >= 0)
			{
				int albedo_img_idx = model.textures[albedo_tex_idx].source;
				image_infos[albedo_img_idx].format.SRGB = 1;
			}
		}
		// actually create the textures
		for (int i = 0; i < model.images.size(); i++)
		{
			auto& img = model.images[i];
			auto tex = new Texture2D(
				img.name,
				img.image.data(),
				img.width, img.height,
				image_infos[i].format);
			image_infos[i].texture = tex;
			asset_textures.push_back(tex);
		}
#endif

		// materials
		std::vector<std::string> texture_names(model.textures.size());
		for (int i = 0; i < model.textures.size(); i++) {
			texture_names[i] = model.images[model.textures[i].source].name;
		}

		std::vector<std::string> material_names(model.materials.size());
		for (int i = 0; i < model.materials.size(); i++) {
			auto& mat = model.materials[i];
			material_names[i] = mat.name;

			// create mat info and add it to the mapping

			int albedo_idx = mat.pbrMetallicRoughness.baseColorTexture.index;
			auto albedo = albedo_idx >= 0 ? texture_names[albedo_idx] : "_white";

			int normal_idx = mat.normalTexture.index;
			auto normal = normal_idx >= 0 ? texture_names[normal_idx] : "_defaultNormal";

			int mr_idx = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
			auto metallic_roughness = mr_idx >= 0 ? texture_names[mr_idx] : "_white";

			int ao_idx = mat.occlusionTexture.index;
			auto ao = ao_idx >= 0 ? texture_names[ao_idx] : "_white";

			auto bc = mat.pbrMetallicRoughness.baseColorFactor;
			auto baseColorFactor = glm::vec4(bc[0], bc[1], bc[2], bc[3]);
			glm::vec4 strengths = {
				(float)mat.occlusionTexture.strength,
				(float)mat.pbrMetallicRoughness.roughnessFactor,
				(float)mat.pbrMetallicRoughness.metallicFactor,
				(float)mat.normalTexture.scale
			};
			auto em = mat.emissiveFactor;
			glm::vec4 emissiveFactor = glm::vec4(em[0], em[1], em[2], 1);

			GltfMaterialInfo info = {
				._version = 0,
				.type = MaterialType::Surface,
				.name = mat.name,
				.albedoTexName = albedo,
				.normalTexName = normal,
				.mrTexName = metallic_roughness,
				.aoTexName = ao,
				.BaseColorFactor = baseColorFactor,
				.EmissiveFactor = emissiveFactor,
				.OcclusionRoughnessMetallicNormalStrengths = strengths,
				.volumeColor = glm::vec4(0, 0, 0, 0),
				.volumeDensity = 0
			};

			// and in case it's a volume material...
			tinygltf::Value isVolume;
			tinygltf::Value volumeColor;
			tinygltf::Value volumeDensity;
			if (findMaterialProperty(mat, "_is_volume", isVolume) &&
				findMaterialProperty(mat, "_volume_color", volumeColor) &&
				findMaterialProperty(mat, "_volume_density", volumeDensity))
			{
				info.type = MaterialType::Volume;
				info.volumeColor = glm::vec4(
					volumeColor.Get(0).GetNumberAsDouble(),
					volumeColor.Get(1).GetNumberAsDouble(),
					volumeColor.Get(2).GetNumberAsDouble(),
					volumeColor.Get(3).GetNumberAsDouble());
				info.volumeDensity = (float)volumeDensity.GetNumberAsDouble();
			}
			GltfMaterialInfo::add(info);
		}

		//====================

		std::unordered_map<PrimitiveBufferIndex, Mesh::CpuDataAccessor> cpu_buffer_indices;
#if GRAPHICS_DISPLAY
		std::unordered_map<PrimitiveBufferIndex, Mesh::GpuDataAccessor> gpu_buffer_indices;
#endif
		load_mesh_buffers(
			model,
			cpu_buffer_indices,
			combined_vertices,
			combined_indices
#if GRAPHICS_DISPLAY
			, &gpu_buffer_indices,
			&combined_vertex_buffer,
			&combined_index_buffer
#endif
		);

		//====================

		// camera, mesh
		auto tree = loadSceneTree(model.nodes);

		{// light
			std::unordered_map<std::string, SceneNodeIntermediate*> nodes_map;
			tree->foreach([&nodes_map](SceneNodeIntermediate* node) { nodes_map[node->name] = node; });
			for (int i = 0; i < model.lights.size(); i++) {
				auto& light_name = model.lights[i].name;
				if (nodes_map.contains(light_name)) {
					nodes_map[light_name]->light_idx = i;
				}
			}
		}

		if (Config->lookup<int>("Debug.CollapseSceneTree")) collapseSceneTree(tree);

		// actually construct the scene tree

		std::unordered_map<SceneNodeIntermediate*, SceneObject*> nodeToDrawable;
		std::queue<SceneNodeIntermediate*> nodesQueue;
		nodesQueue.push(tree);
		while (!nodesQueue.empty())
		{
			auto node = nodesQueue.front();
			nodesQueue.pop();

			SceneObject *object = nullptr;

			if (node->camera_idx != -1)
			{
				object = new Camera(node->name, &model.cameras[node->camera_idx]);
			}
			else if (node->light_idx != -1)
			{
				tinygltf::Light *in_light = &model.lights[node->light_idx];
				if (in_light->type == "point")
				{
					object = new PointLight(node->name, in_light);
				}
				else if (in_light->type == "directional")
				{
					object = new DirectionalLight(node->name, in_light);
				}
				else
				{
					WARN("Unsupported light (%s : %s)", in_light->name.c_str(), in_light->type.c_str())
					object = new SceneObject(nullptr, node->name);
				}
			}
			else if (node->mesh_idx != -1)
			{
				auto in_mesh = &model.meshes[node->mesh_idx];
				std::vector<Mesh*> meshes = load_gltf_meshes(
					node->name,
					in_mesh,
					material_names,
					cpu_buffer_indices
#if GRAPHICS_DISPLAY
					, gpu_buffer_indices
#endif
					);
				if (in_mesh->primitives.size() > 1) {
					object = new SceneObject(nullptr, in_mesh->name);
					for (auto m : meshes)
					{
						object->add_child(new MeshObject(m));
					}
				}
				else {
					object = new MeshObject(meshes[0]);
				}
			}
			else {
				object = new SceneObject(nullptr, "[transform] " + node->name);
			}
			nodeToDrawable[node] = object;

			glm::vec3 position;
			glm::quat rotation;
			glm::vec3 scale;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(node->transformation, scale, rotation, position, skew, perspective);

			object->set_local_position(object->local_position() + position);
			object->set_rotation(rotation * object->rotation());
			object->set_scale(object->scale() * scale);

			if (node->parent)
			{
				nodeToDrawable[node->parent]->add_child(object);
			}

			for (auto child : node->children)
			{
				nodesQueue.push(child);
			}
		}

		asset_root = nodeToDrawable[tree];
#if GRAPHICS_DISPLAY
		asset_root->ui_default_open = true;
#endif
		if (outer_root) outer_root->add_child(asset_root);

		delete tree;
	};

	reload();
}

void SceneAsset::release_resources()
{
#if GRAPHICS_DISPLAY
	for (auto tex : asset_textures) {
		delete tex;
	}
	asset_textures.clear();

	combined_vertices.clear();
	combined_indices.clear();
	combined_vertex_buffer.release();
	combined_index_buffer.release();
#endif
	Asset::release_resources();
}

////////////////////// MeshAsset /////////////////////////

std::unordered_map<std::string, std::string> alias_pool;

MeshAsset::MeshAsset(const std::string &relative_path, const std::string &alias)
	: Asset(relative_path, nullptr)
{
	alias_pool[alias] = relative_path;

	// reload is disabled for now
	reload_condition = [](){ return false; };

	load_action_internal = [this, relative_path](){

		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		loader.SetPreserveImageChannels(false);
		std::string err;
		std::string warn;

		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, ROOT_DIR"/" + relative_path);

		if (!warn.empty()) WARN("[TinyGLTF] %s", warn.c_str())
		if (!err.empty()) ERR("[TinyGLTF] %s", err.c_str())
		if (!ret) ERR("[TinyGLTF] Failed to parse glTF")

		std::unordered_map<PrimitiveBufferIndex, Mesh::CpuDataAccessor> cpu_buffer_indices;
#if GRAPHICS_DISPLAY
		std::unordered_map<PrimitiveBufferIndex, Mesh::GpuDataAccessor> gpu_buffer_indices;
#endif
		load_mesh_buffers(
			model,
			cpu_buffer_indices,
			combined_vertices,
			combined_indices
#if GRAPHICS_DISPLAY
			, &gpu_buffer_indices,
			&combined_vertex_buffer,
			&combined_index_buffer
#endif
		);


		for (auto & in_mesh : model.meshes) {

			if (in_mesh.primitives.size() != 1) {
				WARN("mesh '%s' has more than 1 primitive; only the first one will be loaded", in_mesh.name.c_str())
			}
			auto& prim = in_mesh.primitives[0];
			if (prim.mode != TINYGLTF_MODE_TRIANGLES) {
				WARN("'%s' contains unsupported mesh mode %d. skipping..", in_mesh.name.c_str(), prim.mode)
				continue;
			}

			auto buf_idx = primitive_buffer_indices(prim);
			mesh = new Mesh(in_mesh.name, "");
			mesh->cpu_data = cpu_buffer_indices.at(buf_idx);
#if GRAPHICS_DISPLAY
			mesh->gpu_data = gpu_buffer_indices.at(buf_idx);
#endif
			LOG("loading shared mesh asset '%s'", in_mesh.name.c_str())
		}

	};
	reload();
}

Mesh *MeshAsset::find(const std::string &alias)
{
	auto rel_path = alias_pool[alias];
	auto* ma = Asset::find<MeshAsset>(rel_path);
	return ma->mesh;
}

void MeshAsset::release_resources()
{
	delete mesh;
	mesh = nullptr;

	combined_vertices.clear();
	combined_indices.clear();
#if GRAPHICS_DISPLAY
	combined_vertex_buffer.release();
	combined_index_buffer.release();
#endif
	Asset::release_resources();
}
