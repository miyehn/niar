//
// Created by raind on 5/22/2022.
//

#include "Scene/Scene.hpp"
#include "Scene/Camera.hpp"
#include "Scene/Light.hpp"
#include "Render/Mesh.h"
#include "Render/Texture.h"
#include "Render/Materials/GltfMaterial.h"
#include "Engine/ConfigAsset.hpp"
#include "GltfAsset.h"
#include "SceneObject.hpp"

#include <glm/gtx/matrix_decompose.hpp>
#include <tinygltf/tiny_gltf.h>

#include <unordered_map>
#include <queue>

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
}

GltfAsset::GltfAsset(
	SceneObject* outer_root,
	const std::string &relative_path,
	const std::function<bool()> &reload_condition)
: Asset(relative_path,nullptr)
{
	load_action = [this, outer_root, relative_path, reload_condition]() {

		// cleanup first, if necessary
		outer_root->try_remove_child(asset_root);
		Vulkan::Instance->waitDeviceIdle();
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
				(float)mat.pbrMetallicRoughness.metallicFactor,
				(float)mat.pbrMetallicRoughness.roughnessFactor,
				(float)mat.occlusionTexture.strength,
				(float)mat.normalTexture.scale
			};
			auto em = mat.emissiveFactor;
			glm::vec4 emissiveFactor = glm::vec4(em[0], em[1], em[2], 1);

			GltfMaterialInfo info = {
				._version = 0,
				.name = mat.name,
				.albedoTexName = albedo,
				.normalTexName = normal,
				.mrTexName = metallic_roughness,
				.aoTexName = ao,
				.BaseColorFactor = baseColorFactor,
				.EmissiveFactor = emissiveFactor,
				.MetallicRoughnessAONormalStrengths = strengths
			};
			GltfMaterial::addInfo(info);
		}

		//====================

		// camera, mesh
		auto tree = loadSceneTree(model.nodes);

		{// light
			std::unordered_map<std::string, SceneNodeIntermediate*> nodes_map;
			tree->foreach([&nodes_map](SceneNodeIntermediate* node) { nodes_map[node->name] = node; });
			for (int i = 0; i < model.lights.size(); i++) {
				nodes_map[model.lights[i].name]->light_idx = i;
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
				std::vector<Mesh*> meshes = Mesh::load_gltf(node->name, in_mesh, &model, material_names);
				if (in_mesh->primitives.size() > 1)
				{
					object = new SceneObject(nullptr, in_mesh->name);
					for (auto m : meshes)
					{
						m->initialize_gpu();
						object->add_child(m);
					}
				}
				else
				{
					meshes[0]->initialize_gpu();
					object = meshes[0];
				}
			}
			else
			{
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
		outer_root->add_child(asset_root);

		delete tree;
	};
	this->reload_condition = reload_condition;

	reload();
}

void GltfAsset::release_resources()
{
	for (auto tex : asset_textures) {
		delete tex;
	}
	asset_textures.clear();
}
