#include "Scene.hpp"
#include "Assets/ConfigAsset.hpp"
#include "Render/Mesh.h"

#include "Utils/DebugUI.h"
#include "MeshObject.h"
#include "Utils/myn/RenderDoc.h"

// TODO: make this support parenting hierarchy
std::vector<MeshObject*> Scene::get_meshes() {
	std::vector<MeshObject*> meshes;
	for (int i=0; i<children.size(); i++) {
		if (MeshObject* mo = dynamic_cast<MeshObject*>(children[i])) meshes.push_back(mo);
	}
	return meshes;
}

void Scene::generate_aabb() {
	std::vector<MeshObject*> meshes = get_meshes();
	aabb = AABB();
	for (auto & mesh : meshes) {
		aabb.merge(mesh->aabb);
	}
}

Scene::Scene(const std::string &_name) : SceneObject(nullptr, _name)
{
}

bool Scene::handle_event(SDL_Event event) {
	if (event.type == SDL_EventType::SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_PRINTSCREEN) {
			myn::RenderDoc::captureNextFrame();
			LOG("capturing with renderdoc..")
		}
	}
	SceneObject::handle_event(event);
	return true;
}

