#include "Texture.h"
#include <stb_image/stb_image.h>
#include "Render/Vulkan/VulkanUtils.h"

// std::unordered_map<std::string, Texture *> Texture::pool;

std::unordered_map<std::string, Texture *>& texturePool()
{
	static std::unordered_map<std::string, Texture *> pool;
	/*
	static bool firstEntrance = true;
	if (firstEntrance) {
		Vulkan::Instance->destructionQueue.emplace_back([](){
			for (auto &tex : pool)
				delete tex.second;
		});
		firstEntrance = false;
	}
	 */
	return pool;
}

Texture *Texture::get(const std::string &path)
{
	auto it = texturePool().find(path);
	if (it != texturePool().end()) return (*it).second;

	WARN("retrieving texture '%s' before it is added to the pool. Returning black dummy..", path.c_str())
	return texturePool().find("_black")->second;
}

Texture::~Texture()
{
	vmaDestroyImage(Vulkan::Instance->memoryAllocator, resource.image, resource.allocation);
}

//--------

void createTexture2DFromPixelData(
	uint8_t *pixels,
	uint32_t width,
	uint32_t height,
	VkFormat imageFormat,
	uint32_t pixelSize,
	bool generateMips,
	VmaAllocatedImage &outResource,
	VkImageView &outImageView)
{
	VkExtent3D imageExtent = {
		.width = static_cast<uint32_t>(width),
		.height = static_cast<uint32_t>(height),
		.depth = 1
	};

	uint32_t numMips = generateMips ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1 : 1;

	VkImageCreateInfo imgInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = imageFormat,
		.extent = imageExtent,
		.mipLevels = numMips,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
	};

	if (generateMips) imgInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo imgAllocInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY
	};

	vmaCreateImage(Vulkan::Instance->memoryAllocator, &imgInfo, &imgAllocInfo, &outResource.image, &outResource.allocation, nullptr);

	vk::uploadPixelsToImage(pixels, 0, 0, width, height, pixelSize, outResource); // to mip 0
	if (generateMips)
	{
		vk::generateMips(outResource, width, height);
	}

	// create image view
	VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.flags = 0,
		.image = outResource.image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = imageFormat,
		.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = numMips,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	EXPECT(vkCreateImageView(Vulkan::Instance->device, &viewInfo, nullptr, &outImageView), VK_SUCCESS)
}

#include "TextureFormatMappings.inl"

Texture2D::Texture2D(const std::string &name, const std::string &path, ImageFormat textureFormat)
{
#ifdef DEBUG
	auto it = texturePool().find(path);
	if (it != texturePool().end()) WARN("trying to load texture '%s' that's already in the pool. Overriding..", path.c_str())
	EXPECT(textureFormat.channelDepth % 8, 0)
#endif

	int native_channels;
	int iwidth, iheight;
	uint8_t* pixels = nullptr;
	if (textureFormat.channelDepth==8) {
		pixels = stbi_load(path.c_str(), &iwidth, &iheight, &native_channels, textureFormat.numChannels);
	} else if (textureFormat.channelDepth==16) {
		pixels = (uint8_t*)stbi_load_16(path.c_str(), &iwidth, &iheight, &native_channels, textureFormat.numChannels);
	} else if (textureFormat.channelDepth==32) {
		pixels = (uint8_t*)stbi_loadf(path.c_str(), &iwidth, &iheight, &native_channels, textureFormat.numChannels);
	} else {
		ERR("Trying to load image '%s' with wrong channelDepth", path.c_str())
	}

	//LOG("load texture '%s' %dx%dx%d", relative_path.c_str(), iwidth, iheight, native_channels)
	EXPECT(pixels != nullptr, true)

	imageFormat = getFormatFromMap(textureFormat);

	width = iwidth;
	height = iheight;
	num_slices = 1;

	uint32_t pixelSize = textureFormat.numChannels * (textureFormat.channelDepth / 8);
	createTexture2DFromPixelData(pixels, width, height, imageFormat, pixelSize, true, resource, imageView);

	NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, resource.image, name)
	NAME_OBJECT(VK_OBJECT_TYPE_IMAGE_VIEW, imageView, name + "_defaultView")

	stbi_image_free(pixels);

	texturePool()[name] = this;
}

void Texture2D::createDefaultTextures()
{
	static bool createdDefaultTextures = false;

	if (createdDefaultTextures)
	{
		WARN("Trying to re-create default textures. Skipping..")
		return;
	}

	std::vector<Texture2D*> global_textures;

	auto* whiteTexture = new Texture2D();
	whiteTexture->imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	whiteTexture->num_slices = 1;
	whiteTexture->width = 1;
	whiteTexture->height = 1;
	uint8_t whitePixel[] = {255, 255, 255, 255};
	createTexture2DFromPixelData(
		whitePixel, 1, 1,
		VK_FORMAT_R8G8B8A8_UNORM,
		4,
		false,
		whiteTexture->resource,
		whiteTexture->imageView);
	texturePool()["_white"] = whiteTexture;
	global_textures.push_back(whiteTexture);
	NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, whiteTexture->resource.image, "_white")

	auto* blackTexture = new Texture2D();
	blackTexture->imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	blackTexture->num_slices = 1;
	blackTexture->width = 1;
	blackTexture->height = 1;
	uint8_t blackPixel[] = {0, 0, 0, 0};
	createTexture2DFromPixelData(
		blackPixel, 1, 1,
		VK_FORMAT_R8G8B8A8_UNORM,
		4,
		false,
		blackTexture->resource,
		blackTexture->imageView);
	texturePool()["_black"] = blackTexture;
	global_textures.push_back(blackTexture);
	NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, blackTexture->resource.image, "_black")

	auto* defaultNormal = new Texture2D();
	defaultNormal->imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	defaultNormal->num_slices = 1;
	defaultNormal->width = 1;
	defaultNormal->height = 1;
	uint8_t defaultNormalPixel[] = {127, 127, 255, 0};
	createTexture2DFromPixelData(
		defaultNormalPixel, 1, 1,
		VK_FORMAT_R8G8B8A8_UNORM,
		4,
		false,
		defaultNormal->resource,
		defaultNormal->imageView);
	texturePool()["_defaultNormal"] = defaultNormal;
	global_textures.push_back(defaultNormal);
	NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, defaultNormal->resource.image, "_defaultNormal")

	Vulkan::Instance->destructionQueue.emplace_back([global_textures](){
		for (auto tex : global_textures)
			delete tex;
	});
}

Texture2D::Texture2D(ImageCreator &imageCreator)
{
	imageFormat = imageCreator.imageInfo.format;
	width = imageCreator.imageInfo.extent.width;
	height = imageCreator.imageInfo.extent.height;
	num_slices = imageCreator.imageInfo.extent.depth;

	imageCreator.create(resource, imageView);
	if (imageCreator.debugName.length() > 0)
	{
		NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, resource.image, imageCreator.debugName)
		NAME_OBJECT(VK_OBJECT_TYPE_IMAGE_VIEW, imageView, imageCreator.debugName + "_defaultView")
	}
}

Texture2D::Texture2D(const std::string &name, uint8_t *data, uint32_t width, uint32_t height, ImageFormat format)
{
	LOG("loading texture '%s'..", name.c_str())

	imageFormat = getFormatFromMap(format);
	this->width = width;
	this->height = height;
	this->num_slices = 1;

	createTexture2DFromPixelData(
		data, width, height,
		imageFormat,
		(format.numChannels * format.channelDepth / 8),
		true,
		resource,
		imageView);
	texturePool()[name] = this;

	NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, resource.image, name)
	NAME_OBJECT(VK_OBJECT_TYPE_IMAGE_VIEW, imageView, name + "_defaultView")
}

Texture2D::~Texture2D()
{
	vkDestroyImageView(Vulkan::Instance->device, imageView, nullptr);
}
