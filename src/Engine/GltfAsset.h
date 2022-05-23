//
// Created by raind on 5/22/2022.
//

#pragma once

#include "Asset.h"
#include <vector>

class SceneObject;
class Texture2D;

class GltfAsset : public Asset
{
public:
	explicit GltfAsset(
		SceneObject* outer_root,
		const std::string& relative_path,
		const std::function<bool()>& reload_condition);

	~GltfAsset() override { release_resources(); }

	void release_resources();

private:

	SceneObject* asset_root = nullptr;
	std::vector<Texture2D*> asset_textures;
};

