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

	SceneObject* get_root() { return asset_root; }

protected:
	void release_resources() override;

private:

	SceneObject* asset_root = nullptr;

	std::vector<Vertex> combined_vertices;
	std::vector<VERTEX_INDEX_TYPE> combined_indices;

#if GRAPHICS_DISPLAY
	struct MaterialTextureHandleSet {
		BindlessTexture2DHandle albedo;
		BindlessTexture2DHandle normal;
		BindlessTexture2DHandle orm;
		BindlessTexture2DHandle emissive;
	};

	std::vector<Texture2D*> asset_images; // same size as model.images
	std::vector<BindlessTexture2DHandle> asset_texture_handles; // same size as model.textures (may reuse images)
	std::vector<MaterialTextureHandleSet> asset_material_texture_handle_sets; // same size as model.materials

	VmaBuffer combined_vertex_buffer;
	VmaBuffer combined_index_buffer;
	std::vector<BLASInfo> blas_collection; // for each primitive in each mesh. todo [myn]: are instanced meshes duplicated?
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
	std::vector<BLASInfo> blas_collection;
#endif
};
