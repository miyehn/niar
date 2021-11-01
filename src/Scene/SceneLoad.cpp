#include "Scene.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Asset/Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <unordered_map>
#include <queue>

struct SceneNodeIntermediate
{
	// converter
	SceneNodeIntermediate(aiNode* node) :
	transformation(node->mTransformation),
	name(node->mName.C_Str()) {}

	~SceneNodeIntermediate()
	{
		for (auto c : children) delete c;
	}

	// hierarchy
	SceneNodeIntermediate* parent = nullptr;
	std::vector<SceneNodeIntermediate*> children;

	// transformation
	aiMatrix4x4t<float> transformation;

	// data
	std::string name;
	aiLight* light = nullptr;
	aiMesh* mesh = nullptr;
	aiCamera* camera = nullptr;
};

SceneNodeIntermediate* loadSceneTree(const aiScene* scene)
{
	// map from name to node
	// load the tree hierarchy and transformation; construct map
	// use the map to add data
	std::unordered_map<std::string, SceneNodeIntermediate*> nodesMap;

	// hierarchy
	std::queue<aiNode*> nodesQueue;
	nodesQueue.push(scene->mRootNode);
	while (!nodesQueue.empty())
	{
		aiNode* ainode = nodesQueue.front();
		nodesQueue.pop();

		auto tmpnode = new SceneNodeIntermediate(ainode);
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

void collapseSceneTree(SceneNodeIntermediate* root)
{
	if (root == nullptr) return;
	for (auto child : root->children) collapseSceneTree(child);
	if (root->children.size() == 1 &&
		!root->camera &&
		!root->light &&
		!root->mesh)
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

void Scene::load(const std::string& path, bool preserve_existing_objects)
{
	if (!preserve_existing_objects) {
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
	LOG(" - %d cameras", scene->mNumCameras);
	LOG(" - %d lights", scene->mNumLights);
	LOG(" - %d meshes", scene->mNumMeshes);

	SceneNodeIntermediate* sceneTree = loadSceneTree(scene);
	collapseSceneTree(sceneTree);

	//-------- load from intermediate tree --------

	// camera, light, or mesh

	std::unordered_map<SceneNodeIntermediate*, Drawable*> nodeToDrawable;
	std::queue<SceneNodeIntermediate*> nodesQueue;
	nodesQueue.push(sceneTree);
	while (!nodesQueue.empty())
	{
		auto node = nodesQueue.front();
		nodesQueue.pop();

		Drawable* drawable = nullptr;
		if (node->camera)
		{
			aiCamera* camera = node->camera;
			Camera* loaded_camera = new Camera(camera);
			drawable = loaded_camera;
			if (!Camera::Active) Camera::Active = loaded_camera;
			else WARN("There're multiple cameras in the scene. Only one is used as the active one.")
		}
		else if (node->light)
		{
			aiLight* light = node->light;
			if (light->mType == aiLightSource_POINT)
			{
				drawable = new PointLight(light);
			}
			else if (light->mType == aiLightSource_DIRECTIONAL)
			{
				drawable = new DirectionalLight(light);
			}
		}
		else if (node->mesh)
		{
			Mesh* m = new Mesh(node->mesh);
			m->initialize_gpu();
			drawable = m;
		}
		else
		{
			drawable = new SceneObject(nullptr, "[transform node]");
		}

		nodeToDrawable[node] = drawable;

		aiVector3t<float> aiPosition;
		aiQuaterniont<float> aiRotation;
		aiVector3t<float> aiScale;
		node->transformation.Decompose(aiScale, aiRotation, aiPosition);

		vec3 position = vec3(aiPosition.x, aiPosition.y, aiPosition.z);
		quat rotation = quat(aiRotation.w, aiRotation.x, aiRotation.y, aiRotation.z);
		vec3 scale = vec3(aiScale.x, aiScale.y, aiScale.z);

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

	add_child(nodeToDrawable[sceneTree]);

	delete sceneTree;
}