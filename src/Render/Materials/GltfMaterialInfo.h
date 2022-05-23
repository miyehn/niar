//
// Created by raind on 5/24/2022.
//
#pragma once

#include <unordered_map>
#include <glm/glm.hpp>

// used for each renderer to create corresponding materials
struct GltfMaterialInfo {
	uint32_t _version;
	std::string name;
	std::string albedoTexName;
	std::string normalTexName;
	std::string mrTexName;
	std::string aoTexName;
	glm::vec4 BaseColorFactor;
	glm::vec4 EmissiveFactor;
	glm::vec4 MetallicRoughnessAONormalStrengths;

	static void add(GltfMaterialInfo& info);
	static GltfMaterialInfo* get(const std::string& materialName);
};
