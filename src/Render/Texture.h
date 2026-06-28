#pragma once
#include <string>
#include "Render/BindlessResources.h"
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
	virtual ~Texture();

	std::string getName() const { return name; }

	VmaAllocatedImage resource;

protected:
	Texture() = default;

	std::string name;
};

class Texture2D : public Texture
{
public:

	VkImageView imageView;
	VkFormat imageFormat;

	// load from file
	explicit Texture2D(
		const std::string &name,
		const std::string &path,
		ImageFormat textureFormat={4,8,1},
		const BindlessTexture2DInfo& bindlessInfo = {});

	// create with code but provide pixel data (ie. gltf)
	explicit Texture2D(
		const std::string &name,
		uint8_t* data,
		uint32_t width,
		uint32_t height,
		ImageFormat format,
		bool generateMips = true,
		const BindlessTexture2DInfo& bindlessInfo = {}
		);

	// allocate programmatically (NOT POOLED)
	explicit Texture2D(
		ImageCreator &imageCreator,
		const BindlessTexture2DInfo& bindlessInfo = {});

	uint32_t getWidth() const { return width; }
	uint32_t getHeight() const { return height; }
	BindlessTexture2DHandle getBindlessHandle() const { return bindlessHandle; }

	~Texture2D() override;

	static void createDefaultTextures();
	static void cleanupDefaultTextures();
	static Texture2D* white();
	static Texture2D* black();
	static Texture2D* defaultNormal();

private:
	uint32_t width;
	uint32_t height;

	BindlessTexture2DHandle bindlessHandle;

	void registerBindless(const BindlessTexture2DInfo& bindlessInfo);
	void unregisterBindless();

#if TMP_BINDLESS_DEBUG
	static void runBindlessLifetimeSelfTest();
#endif
};
