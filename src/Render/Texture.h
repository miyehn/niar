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
	static Texture* get(const std::string &path);
	virtual ~Texture();

	VmaAllocatedImage resource;

protected:
	Texture() = default;
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
		ImageFormat format
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