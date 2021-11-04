#include "Texture.h"
#include <stb_image/stb_image.h>
#include "Render/Vulkan/VulkanUtils.h"

std::unordered_map<std::string, Texture *> Texture::pool;

void Texture::cleanup()
{
	for (auto & it : Texture::pool)
	{
		delete it.second;
	}
}

Texture *Texture::get(const std::string &path)
{
	auto it = Texture::pool.find(path);
	if (it != Texture::pool.end()) return (*it).second;

	WARN("retrieving texture '%s' before it is added to the pool. Trying to load from file with default settings (SRGB)..",
		 path.c_str())
	auto new_texture = new Texture2D(path);
	Texture::pool[path] = new_texture;
	return new_texture;
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
	VmaAllocatedImage &outResource,
	VkImageView &outImageView)
{
	VkExtent3D imageExtent = {
		.width = static_cast<uint32_t>(width),
		.height = static_cast<uint32_t>(height),
		.depth = 1
	};

	VkImageCreateInfo imgInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = imageFormat,
		.extent = imageExtent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
	};

	VmaAllocationCreateInfo imgAllocInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY
	};

	vmaCreateImage(Vulkan::Instance->memoryAllocator, &imgInfo, &imgAllocInfo, &outResource.image, &outResource.allocation, nullptr);

	vk::uploadPixelsToImage(pixels, 0, 0, width, height, pixelSize, outResource);

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
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	EXPECT(vkCreateImageView(Vulkan::Instance->device, &viewInfo, nullptr, &outImageView), VK_SUCCESS)
}

#include "TextureFormatMappings.inl"

Texture2D::Texture2D(const std::string &path, ImageFormat textureFormat)
{
#ifdef DEBUG
	auto it = Texture::pool.find(path);
	if (it != Texture::pool.end()) WARN("trying to load texture '%s' that's already in the pool. Overriding..", path.c_str())
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

	LOG("load texture w %d h %d channels %d", iwidth, iheight, native_channels)
	EXPECT(pixels != nullptr, true)

	imageFormat = getFormatFromMap(textureFormat);

	width = iwidth;
	height = iheight;
	num_slices = 1;

	uint32_t pixelSize = textureFormat.numChannels * (textureFormat.channelDepth / 8);
	createTexture2DFromPixelData(pixels, width, height, imageFormat, pixelSize,resource,imageView);

	stbi_image_free(pixels);

	Texture::pool[path] = this;
}

void Texture2D::createDefaultTextures()
{
	static bool createdDefaultTextures = false;

	if (createdDefaultTextures)
	{
		WARN("Trying to re-create default textures. Skipping..")
		return;
	}

	auto* whiteTexture = new Texture2D();
	whiteTexture->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
	whiteTexture->num_slices = 1;
	whiteTexture->width = 1;
	whiteTexture->height = 1;
	uint8_t whitePixel[] = {255, 255, 255, 255};
	createTexture2DFromPixelData(
		whitePixel, 1, 1,
		VK_FORMAT_R8G8B8A8_UNORM,
		4,
		whiteTexture->resource,
		whiteTexture->imageView);
	Texture::pool["_white"] = whiteTexture;

	// TODO: more
}

Texture2D::~Texture2D()
{
	vkDestroyImageView(Vulkan::Instance->device, imageView, nullptr);
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
