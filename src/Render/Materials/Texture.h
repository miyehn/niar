#pragma once
#include <string>
#include <unordered_map>
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/ImageCreator.h"

class Texture
{
protected:
	Texture() = default;
	virtual ~Texture();

	static std::unordered_map<std::string, Texture*> pool;

public:
	static Texture* get(const std::string &path);
	static void cleanup();

	VmaAllocatedImage resource;
};

class Texture2D : public Texture
{
protected:
	uint32_t width;
	uint32_t height;
	uint32_t num_slices;

	Texture2D() = default;

public:

	VkImageView imageView;
	VkFormat imageFormat;

	static void createDefaultTextures();

	~Texture2D() override;

	explicit Texture2D(const std::string &path);

	// TODO: also take a string argument and use it to add it to pool?
	explicit Texture2D(ImageCreator &imageCreator);
};