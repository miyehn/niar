//
// Created by raind on 5/22/2022.
//

#pragma once

#include "Asset.h"
#include "Render/Mesh.h"
#if GRAPHICS_DISPLAY
#include "Render/BindlessResources.h"
#include "Render/Vulkan/Buffer.h"
#endif
#include <vector>

class SceneObject;
class Texture2D;

#if GRAPHICS_DISPLAY
struct BLASInfo {
	VkAccelerationStructureKHR blas = VK_NULL_HANDLE;
	VmaBuffer blasBuffer;
};
#endif

/*
 * Currently offline rendering doesn't load textures because managing textures sounds like a pain
 * hmm actually... it's doable?
 * TODO
 */
class SceneAsset : public Asset
{
public:
	explicit SceneAsset(
		SceneObject* outer_root,
		const std::string& relative_path);

	SceneObject* get_root() const { return asset_root; }

protected:
	void release_resources() override;

private:

	SceneObject* asset_root = nullptr;

	std::vector<Vertex> combined_vertices;
	std::vector<VERTEX_INDEX_TYPE> combined_indices;

#if GRAPHICS_DISPLAY
	std::vector<Texture2D*> asset_images; // same size as model.images
	std::vector<uint32_t> asset_material_indices; // same size as model.materials; cleanup tokens and mesh bridge source
	std::vector<uint32_t> asset_geometry_indices; // cleanup tokens for registered GPU geometry records

	VmaBuffer combined_vertex_buffer;
	VmaBuffer combined_index_buffer;

	std::vector<BLASInfo> blas_collection; // one per unique primitive buffer
#endif
};

class MeshAsset : public Asset
{
public:
	explicit MeshAsset(const std::string& relative_path, const std::string& alias);

	static Mesh* find(const std::string& alias);

	Mesh mesh = {};

protected:
	void release_resources() override;

private:

	std::vector<Vertex> combined_vertices;
	std::vector<VERTEX_INDEX_TYPE> combined_indices;

#if GRAPHICS_DISPLAY
	VmaBuffer combined_vertex_buffer;
	VmaBuffer combined_index_buffer;
	std::vector<uint32_t> asset_geometry_indices; // cleanup tokens
	std::vector<BLASInfo> blas_collection;
#endif
};
