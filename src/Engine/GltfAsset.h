//
// Created by raind on 5/22/2022.
//

#pragma once

#include "Asset.h"

class SceneObject;

class GltfAsset : public Asset
{
public:
	explicit GltfAsset(SceneObject* outer_root, const std::string& relative_path, const std::function<void()>& loadAction = nullptr);
private:
	SceneObject* asset_root = nullptr;
};

