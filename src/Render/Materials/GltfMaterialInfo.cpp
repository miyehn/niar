//
// Created by raind on 5/24/2022.
//

#include "GltfMaterialInfo.h"
#include "Utils/myn/Log.h"

namespace
{
std::unordered_map<std::string, GltfMaterialInfo> gltfMaterialInfos;
}

/*
 * info passed in has _version = 0, but should check if it collides with an already-pooled info
 * if so, bump its version before putting into the pool.
 */
void GltfMaterialInfo::add(GltfMaterialInfo &info)
{
	auto iter = gltfMaterialInfos.find(info.name);
	if (iter != gltfMaterialInfos.end())
	{// collided.
		info._version = iter->second._version + 1;
		LOG("Adding tinygltf material of duplicate name '%s'. bumping version..", info.name.c_str())
	}
	gltfMaterialInfos[info.name] = info;
}

GltfMaterialInfo *GltfMaterialInfo::get(const std::string &materialName)
{
	auto iter = gltfMaterialInfos.find(materialName);
	if (iter == gltfMaterialInfos.end())
	{
		WARN("Can't find tinygltf material named '%s'", materialName.c_str())
		return nullptr;
	}
	return &iter->second;
}
