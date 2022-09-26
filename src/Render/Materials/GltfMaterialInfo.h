//
// Created by raind on 5/24/2022.
//
#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

enum MaterialType {
	MT_Surface,
	MT_Volume
};

enum BlendMode {
	BM_OpaqueOrClip,
	BM_AlphaBlend
};

// used for each renderer to create corresponding materials
// https://docs.blender.org/manual/en/2.80/addons/io_scene_gltf2.html
struct GltfMaterialInfo {
	uint32_t _version;
	MaterialType type;
	std::string name;
	std::string albedoTexName;
	std::string normalTexName;
	std::string ormTexName;
	std::string aoTexName;
	glm::vec4 BaseColorFactor;
	glm::vec3 EmissiveFactor;
	glm::vec4 OcclusionRoughnessMetallicNormalStrengths;
	int doubleSided;
	BlendMode blendMode;
	float clipThreshold;
	float volumeDensity;
	glm::vec4 volumeColor;

	static void add(GltfMaterialInfo& info);
	static GltfMaterialInfo* get(const std::string& materialName);
};
