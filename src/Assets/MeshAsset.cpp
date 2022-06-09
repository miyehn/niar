//
// Created by raind on 6/10/2022.
//

#include "Render/Mesh.h"
#include "MeshAsset.h"
#include "Utils/myn/Log.h"

#include <tinygltf/tiny_gltf.h>

namespace
{
std::unordered_map<std::string, std::string> alias_pool;
}

MeshAsset::MeshAsset(const std::string &relative_path, const std::string &alias)
: Asset(relative_path, nullptr), mesh(nullptr)
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

		auto materials_dummy = std::vector<std::string>();

		for (int i = 0; i < model.meshes.size(); i++) {
			auto& in_mesh = model.meshes[i];
			if (in_mesh.primitives.size() != 1) {
				WARN("mesh '%s' has more than 1 primitive; only the first one will be loaded", in_mesh.name.c_str())
			}
			auto& prim = in_mesh.primitives[0];
			if (prim.mode != TINYGLTF_MODE_TRIANGLES) {
				WARN("'%s' contains unsupported mesh mode %d. skipping..", in_mesh.name.c_str(), prim.mode)
				continue;
			}
			auto m = new Mesh(in_mesh.name, &prim, &model, materials_dummy);
			mesh = m;
			ASSET("loading shared mesh asset '%s'", in_mesh.name.c_str())
		}

#if GRAPHICS_DISPLAY
		mesh->initialize_gpu();
#endif
	};
	reload();
}

MeshAsset::~MeshAsset() {
	release_resources();
}

Mesh *MeshAsset::find(const std::string &alias)
{
	auto rel_path = alias_pool[alias];
	auto* ma = dynamic_cast<MeshAsset*>(Asset::find(rel_path));
	return ma->mesh;
}

void MeshAsset::release_resources()
{
	delete mesh;
	mesh = nullptr;
}
