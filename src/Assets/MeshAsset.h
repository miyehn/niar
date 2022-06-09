//
// Created by raind on 6/10/2022.
//

#pragma once
#include "Asset.h"

struct Mesh;

class MeshAsset : public Asset
{
public:
	explicit MeshAsset(const std::string& relative_path, const std::string& alias);
	~MeshAsset() override;

	static Mesh* find(const std::string& alias);

	Mesh* mesh;

	void release_resources();
};
