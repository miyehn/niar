#include "Scene.hpp"
#include "Engine/Program.hpp"
#include "Engine/Input.hpp"
#include "Asset/Mesh.h"
#include "Asset/GlMaterial.h"
#include "Scene/Camera.hpp"

#include "Engine/DebugUI.h"

// OMG MIND BLOWN: https://stackoverflow.com/questions/677620/do-i-need-to-explicitly-call-the-base-virtual-destructor
Scene::~Scene() {
}

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

// TODO: make this support parenting hierarchy
void Scene::draw_content(bool shadow_pass) {
	for (int i=0; i<children.size(); i++) {
#if 0 // front face culling
		if (!shadow_pass) {
			children[i]->draw();
			continue;
		}
		//---- shadow pass ----
		// for each child of type Mesh and is closed mesh
		if (Mesh* mesh = dynamic_cast<Mesh*>(children[i])) {
			if (!mesh->is_thin_mesh) {
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				mesh->draw();
				glCullFace(cull_mode);
				if (!cull_face) glDisable(GL_CULL_FACE);
				continue;
			}
		}
		// non-mesh or not-closed mesh
		children[i]->draw();
#else
		children[i]->draw();
#endif
	}
}

void Scene::draw() {

}

Scene::Scene(const std::string &_name) : Drawable(nullptr, _name)
{

}

