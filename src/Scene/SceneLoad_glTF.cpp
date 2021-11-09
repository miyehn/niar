#include "Scene.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Render/Mesh.h"
#include "Engine/Config.hpp"

#include "Utils/myn/Log.h"

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
			glm::quat rot(in_node.rotation[0], in_node.rotation[1], in_node.rotation[2], in_node.rotation[3]);
			transformation = glm::mat4(rot);
		}
		if (!in_node.scale.empty())
		{
			glm::mat4 scl{};
			scl[0][0] = in_node.scale[0];
			scl[1][1] = in_node.scale[1];
			scl[2][2] = in_node.scale[2];
			scl[3][3] = 1;
			transformation *= scl;
		}
		if (!in_node.translation.empty())
		{
			glm::mat4 tran{};
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
	root->name = "Root Node";
	std::vector<SceneNodeIntermediate*> nodes(in_nodes.size());
	for (int i = 0; i < in_nodes.size(); i++)
	{
		SceneNodeIntermediate* node = new SceneNodeIntermediate(in_nodes[i]);
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
	if (root == nullptr) return;
	for (auto child : root->children) collapseSceneTree(child);
	if (root->children.size() == 1 &&
		root->camera_idx == -1 &&
		root->light_idx == -1 &&
		root->mesh_idx == -1)
	{
		auto oldRoot = root;
		root = *oldRoot->children.begin();
		root->transformation = oldRoot->transformation * root->transformation;
		root->parent = oldRoot->parent;
		if (oldRoot->parent)
		{
			auto& children = oldRoot->parent->children;
			auto iter = std::find(children.begin(), children.end(), oldRoot);
			children.erase(iter);
			children.push_back(root);
		}
		oldRoot->children.clear();
		delete oldRoot;
	}
}
}// namespace

void Scene::load_tinygltf(const std::string &path, bool preserve_existing_objects)
{
	if (!preserve_existing_objects) {
		for (int i=0; i<children.size(); i++) delete children[i];
		children.clear();
	}
	LOG("-------- loading scene (tinygltf) --------");

	using namespace tinygltf;

	Model model;
	TinyGLTF loader;
	loader.SetPreserveImageChannels(true); // instead of widening to 4 channels
	std::string err;
	std::string warn;

	//bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
	bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);

	if (!warn.empty()) WARN("[TinyGLTF] %s", warn.c_str())
	if (!err.empty()) ERR("[TinyGLTF] %s", err.c_str())
	if (!ret) ERR("[TinyGLTF] Failed to parse glTF")

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
	collapseSceneTree(tree);

	// actually construct the scene

	std::unordered_map<SceneNodeIntermediate*, SceneObject*> nodeToDrawable;
	std::queue<SceneNodeIntermediate*> nodesQueue;
	nodesQueue.push(tree);
	while (!nodesQueue.empty())
	{
		auto node = nodesQueue.front();
		nodesQueue.pop();

		SceneObject *drawable = nullptr;

		if (node->camera_idx != -1)
		{
			//...
			drawable = new SceneObject(nullptr, node->name);
		}
		else if (node->light_idx != -1)
		{
			//...
			drawable = new SceneObject(nullptr, node->name);
		}
		else if (node->mesh_idx != -1)
		{
			//...
			drawable = new SceneObject(nullptr, node->name);
		}
		else
		{
			drawable = new SceneObject(nullptr, "[transform] " + node->name);
		}
		nodeToDrawable[node] = drawable;

		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(node->transformation, scale, rotation, position, skew, perspective);

		drawable->set_local_position(drawable->local_position() + position);
		drawable->set_rotation(rotation * drawable->rotation());
		drawable->set_scale(drawable->scale() * scale);

		if (node->parent)
		{
			nodeToDrawable[node->parent]->add_child(drawable);
		}

		for (auto child : node->children)
		{
			nodesQueue.push(child);
		}
	}

	add_child(nodeToDrawable[tree]);

	delete tree;

	// image, texture, sampler, material
}
