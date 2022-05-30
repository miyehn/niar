#include "Scene.hpp"
#include "Engine/ConfigAsset.hpp"
#include "Render/Mesh.h"

#include "Engine/DebugUI.h"

// TODO: make this support parenting hierarchy
std::vector<Mesh*> Scene::get_meshes() {
	std::vector<Mesh*> meshes;
	for (int i=0; i<children.size(); i++) {
		if (Mesh* mesh = dynamic_cast<Mesh*>(children[i])) meshes.push_back(mesh);
	}
	return meshes;
}

void Scene::generate_aabb() {
	std::vector<Mesh*> meshes = get_meshes();
	aabb = AABB();
	for (int i=0; i<meshes.size(); i++) {
		aabb.merge(meshes[i]->aabb);
	}
}

Scene::Scene(const std::string &_name) : SceneObject(nullptr, _name)
{
}

