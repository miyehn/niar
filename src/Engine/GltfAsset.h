//
// Created by raind on 5/22/2022.
//

#pragma once

#include "Asset.h"
#include <vector>

class SceneObject;
class Texture2D;

/*
 * Currently offline rendering doesn't load textures because managing textures sounds like a pain
 */
class GltfAsset : public Asset
{
public:
	explicit GltfAsset(
		SceneObject* outer_root,
		const std::string& relative_path,
		const std::function<bool()>& reload_condition);

	~GltfAsset() override { release_resources(); }

	SceneObject* get_root() { return asset_root; }

	void release_resources();

private:

	SceneObject* asset_root = nullptr;
#if GRAPHICS_DISPLAY
	std::vector<Texture2D*> asset_textures;
#endif
};

