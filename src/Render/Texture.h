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

class Texture
{
public:
	static Texture* get(const std::string &path);

	static void cleanup();

	VmaAllocatedImage resource;

protected:
	Texture() = default;
	virtual ~Texture();

	static std::unordered_map<std::string, Texture*> pool;
};

class Texture2D : public Texture
{
public:

	VkImageView imageView;
	VkFormat imageFormat;

	~Texture2D() override;

	// load from file
	explicit Texture2D(
		const std::string &name,
		const std::string &path,
		ImageFormat textureFormat={4,8,1});

	// create with code but provide pixel data (ie. gltf)
	explicit Texture2D(
		const std::string &name,
		uint8_t* data,
		uint32_t width,
		uint32_t height,
		ImageFormat format
		);

	// allocate programmatically; NOT POOLED
	explicit Texture2D(ImageCreator &imageCreator);

	static void createDefaultTextures();

protected:
	uint32_t width;
	uint32_t height;
	uint32_t num_slices;

	Texture2D() = default;

};