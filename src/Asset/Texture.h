#pragma once
#include <string>
#include <unordered_map>
#include "Render/Vulkan/Vulkan.hpp"

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
	int width;
	int height;
	int num_slices;

	VkImageView imageView;

	Texture2D() = default;
	~Texture2D();

public:
	static void createDefaultTextures();
	explicit Texture2D(const std::string &path);
	VkImageView get_image_view() { return imageView; }
};