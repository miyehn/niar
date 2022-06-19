//
// Created by raind on 5/22/2022.
//

#pragma once

#include "Asset.h"
#include "Render/Mesh.h"
#if GRAPHICS_DISPLAY
#include "Render/Vulkan/Buffer.h"
#endif
#include <vector>

class SceneObject;
class Texture2D;

/*
 * Currently offline rendering doesn't load textures because managing textures sounds like a pain
 */
class SceneAsset : public Asset
{
public:
	explicit SceneAsset(
		SceneObject* outer_root,
		const std::string& relative_path);

	~SceneAsset() override { release_resources(); }

	SceneObject* get_root() { return asset_root; }

	void release_resources();

private:

	SceneObject* asset_root = nullptr;

	std::vector<Vertex> combined_vertices;
	std::vector<VERTEX_INDEX_TYPE> combined_indices;

#if GRAPHICS_DISPLAY
	std::vector<Texture2D*> asset_textures;
	VmaBuffer combined_vertex_buffer;
	VmaBuffer combined_index_buffer;
#endif
};

class MeshAsset : public Asset
{
public:
	explicit MeshAsset(const std::string& relative_path, const std::string& alias);
	~MeshAsset() override;

	static Mesh* find(const std::string& alias);

	Mesh* mesh;

	void release_resources();

private:

	std::vector<Vertex> combined_vertices;
	std::vector<VERTEX_INDEX_TYPE> combined_indices;

#if GRAPHICS_DISPLAY
	VmaBuffer combined_vertex_buffer;
	VmaBuffer combined_index_buffer;
#endif
};
