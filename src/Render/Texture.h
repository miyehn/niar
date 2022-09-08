#pragma once
#include <string>
#include <unordered_map>
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/ImageCreator.h"

struct ImageFormat {
	int numChannels;
	int channelDepth;
	int SRGB;
};

/*
 * Pooled textures: textures loaded from file / gltf asset;
 */
class Texture
{
public:
	template<typename T>
	static T* get(const std::string &path) {
		auto it = texturePool.find(path);
		if (it != texturePool.end()) return dynamic_cast<T*>((*it).second);

		WARN("retrieving texture '%s' before it is added to the pool. Returning black dummy..", path.c_str())
		return dynamic_cast<T*>(texturePool.find("_black")->second);
	}
	virtual ~Texture();

	VmaAllocatedImage resource;

protected:
	Texture() = default;
	static std::unordered_map<std::string, Texture *> texturePool;
};

class Texture2D : public Texture
{
public:

	VkImageView imageView;
	VkFormat imageFormat;

	// load from file (POOLED)
	explicit Texture2D(
		const std::string &name,
		const std::string &path,
		ImageFormat textureFormat={4,8,1});

	// create with code but provide pixel data (ie. gltf) (POOLED)
	explicit Texture2D(
		const std::string &name,
		uint8_t* data,
		uint32_t width,
		uint32_t height,
		ImageFormat format,
		bool generateMips = true
		);

	// allocate programmatically (NOT POOLED)
	explicit Texture2D(ImageCreator &imageCreator);

	~Texture2D() override;

	static void createDefaultTextures(); // (POOLED)

protected:
	uint32_t width;
	uint32_t height;
	uint32_t num_slices;

	Texture2D() = default;

};