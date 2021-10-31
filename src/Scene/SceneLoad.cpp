#include "Scene.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <unordered_map>
#include <queue>

struct NodeTmp
{
	// converter
	NodeTmp(aiNode* node) :
	transformation(node->mTransformation),
	name(node->mName.C_Str()) {}

	~NodeTmp()
	{
		for (auto c : children) delete c;
	}

	// hierarchy
	NodeTmp* parent = nullptr;
	std::vector<NodeTmp*> children;

	// transformation
	aiMatrix4x4t<ai_real> transformation;

	// data
	std::string name;
	aiLight* light = nullptr;
	aiMesh* mesh = nullptr;
	aiCamera* camera = nullptr;
};

NodeTmp* loadSceneTree(const aiScene* scene)
{
	// map from name to node
	// load the tree hierarchy and transformation; construct map
	// use the map to add data
	std::unordered_map<std::string, NodeTmp*> nodesMap;

	// hierarchy
	std::queue<aiNode*> nodesQueue;
	nodesQueue.push(scene->mRootNode);
	while (!nodesQueue.empty())
	{
		aiNode* ainode = nodesQueue.front();
		nodesQueue.pop();

		auto tmpnode = new NodeTmp(ainode);
		nodesMap[ainode->mName.C_Str()] = tmpnode;
		if (ainode->mParent)
		{
			tmpnode->parent = nodesMap[ainode->mParent->mName.C_Str()];
			tmpnode->parent->children.push_back(tmpnode);
		}

		for (auto i = 0; i < ainode->mNumChildren; i++)
		{
			auto aichild = ainode->mChildren[i];
			nodesQueue.push(aichild);
		}
	}

	// data
	for (auto i = 0; i < scene->mNumCameras; i++)
	{
		aiCamera* camera = scene->mCameras[i];
		nodesMap[camera->mName.C_Str()]->camera = camera;
	}
	for (auto i = 0; i < scene->mNumLights; i++)
	{
		aiLight* light = scene->mLights[i];
		nodesMap[light->mName.C_Str()]->light = light;
	}
	for (auto i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[i];
		nodesMap[mesh->mName.C_Str()]->mesh = mesh;
	}

	return nodesMap[scene->mRootNode->mName.C_Str()];
}

void collapseSceneTree(NodeTmp* root)
{
	if (root == nullptr) return;
	if (root->children.size() == 1)
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
	for (auto child : root->children) collapseSceneTree(child);
}

void Scene::load(const std::string& path, bool preserve_existing_objects)
{
	if (!preserve_existing_objects) {
		d_lights.clear();
		p_lights.clear();
		for (int i=0; i<children.size(); i++) delete children[i];
		children.clear();
	}
	LOG("-------- loading scene --------");
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		path,
		// aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_FlipUVs
		// aiProcess_JoinIdenticalVertices |
		// aiProcess_SortByPType
	);
	if (!scene) {
		ERR("%s", importer.GetErrorString());
	}
	LOG(" - %d meshes", scene->mNumMeshes);
	LOG(" - %d lights", scene->mNumLights);

	NodeTmp* sceneTree = loadSceneTree(scene);
	collapseSceneTree(sceneTree);

	//-------- load from intermediate tree --------

	// camera, light, or mesh

	delete sceneTree;
}